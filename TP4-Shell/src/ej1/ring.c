#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int is_last_in_ring(int i, int start, int n) {
    return i == (start + n - 1) % n;
}

void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: anillo <n> <c> <s>\n");
        return 1;
    }

    int n = atoi(argv[1]);
    int val = atoi(argv[2]);
    int start = atoi(argv[3]);

    if (n < 3 || start < 0 || start >= n) {
        fprintf(stderr, "Error: parámetros inválidos\n");
        return 1;
    }

    printf("Se crearán %d procesos, se enviará el caracter %d desde proceso %d\n", n, val, start);

    int pipes[n][2], parent_pipe[2];
    for (int i = 0; i < n; i++)
        if (pipe(pipes[i]) == -1) error_exit("pipe");

    if (pipe(parent_pipe) == -1) error_exit("pipe padre");

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == -1) error_exit("fork");

        if (pid == 0) { // Proceso hijo
            for (int j = 0; j < n; j++) {
                if (j != i) close(pipes[j][0]);
                if (j != (i + 1) % n) close(pipes[j][1]);
            }
            close(parent_pipe[0]);
            if (!is_last_in_ring(i, start, n)) close(parent_pipe[1]);

            read(pipes[i][0], &val, sizeof(int));
            val++;

            if (is_last_in_ring(i, start, n))
                write(parent_pipe[1], &val, sizeof(int));
            else
                write(pipes[(i + 1) % n][1], &val, sizeof(int));

            exit(0);
        }
    }

    // Proceso padre
    write(pipes[start][1], &val, sizeof(int));

    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    close(parent_pipe[1]);

    read(parent_pipe[0], &val, sizeof(int));
    close(parent_pipe[0]);

    for (int i = 0; i < n; i++)
        wait(NULL);

    printf("Resultado final: %d\n", val);
    return 0;
}
