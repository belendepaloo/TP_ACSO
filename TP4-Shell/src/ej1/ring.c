#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]);

int main(int argc, char **argv) {
    int start, status, pid, n;
    int buffer[1];

    if (argc != 4) { printf("Uso: anillo <n> <c> <s> \n"); exit(1); }

    n = atoi(argv[1]);
    buffer[0] = atoi(argv[2]);
    start = atoi(argv[3]);

    if (n < 3) {
        fprintf(stderr, "Error: se requieren al menos 3 procesos para formar un anillo.\n");
        exit(1);
    }

    if (start < 0 || start >= n) {
        fprintf(stderr, "Error: índice de inicio no válido\n");
        exit(1);
    }

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);

    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    int parent_pipe[2];
    if (pipe(parent_pipe) == -1) {
        perror("pipe padre");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
		pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(1);
		} else if (pid == 0) {
			for (int j = 0; j < n; j++) {
				if (j != i) close(pipes[j][0]); 
				if (j != (i + 1) % n) close(pipes[j][1]); 
			}
			close(parent_pipe[0]); 
			if (i != (start + n - 1) % n)
				close(parent_pipe[1]); 

			int val;
			read(pipes[i][0], &val, sizeof(int));
			val++;
			if (i == (start + n - 1) % n) {
				write(parent_pipe[1], &val, sizeof(int));
			} else {
				write(pipes[(i + 1) % n][1], &val, sizeof(int));
			}
			exit(0);
		}
	}

    // 🔧 FIX: el padre debe escribir el valor después de que todos los hijos se hayan creado
    write(pipes[start][1], &buffer[0], sizeof(int));

    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    close(parent_pipe[1]);

    int resultado_final;
    read(parent_pipe[0], &resultado_final, sizeof(int));
    close(parent_pipe[0]);

    for (int i = 0; i < n; i++) {
        wait(&status);
    }

    printf("Resultado final: %d\n", resultado_final);
    return 0;
}

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]) {
    for (int j = 0; j < n; j++) {
        if (j != (i - 1 + n) % n) close(pipes[j][0]); // solo leo del anterior
        if (j != i) close(pipes[j][1]); // solo escribo al siguiente
    }

    close(parent_pipe[0]); // nunca leo del padre
    if (i != start) close(pipes[start][0]); // solo el proceso start lee del padre
    if ((i + 1) % n != start) close(parent_pipe[1]); // solo el último escribe al padre

    int val;

    if (i == start) {
        // 🔧 FIX: leo del padre
        read(pipes[i][0], &val, sizeof(int));
        write(pipes[i][1], &val, sizeof(int)); // envío al siguiente

        // luego recibe del anterior al final de la vuelta
        read(pipes[(i - 1 + n) % n][0], &val, sizeof(int));
        val++;
        write(parent_pipe[1], &val, sizeof(int)); // manda al padre
    } else {
        read(pipes[(i - 1 + n) % n][0], &val, sizeof(int));
        val++;
        write(pipes[i][1], &val, sizeof(int));
    }

    exit(0);
}
