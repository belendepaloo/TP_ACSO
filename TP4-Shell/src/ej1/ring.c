#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]);

int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */
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
    
   	/* You should start programming from here... */
	int pipes[n][2]; // pipes[i]: el proceso i escribe, y el proceso (i+1)%n lee
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
    	if (pid < 0) {
    	    perror("fork");
    	    exit(1);
    	}
    	if (pid == 0) {
    	    child_process(i, n, start, pipes, parent_pipe);
    	}
	}


	// Código del proceso padre

	write(pipes[start][1], &buffer[0], sizeof(int));

	// Cierro los extremos de pipes que no uso
	for (int i = 0; i < n; i++) {
	    close(pipes[i][0]);
	    close(pipes[i][1]);
	}
	close(parent_pipe[1]); // Solo leo del hijo

	// Leer resultado final desde el pipe especial
	int resultado_final;
	read(parent_pipe[0], &resultado_final, sizeof(int));

	printf("Resultado final: %d\n", resultado_final);

	// Esperar que terminen los hijos
	for (int i = 0; i < n; i++) {
	    wait(&status); 
	}

	return 0;
}

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]) {
    // Cerrar todos los pipes que no necesito
    for (int j = 0; j < n; j++) {
        if (j != i) close(pipes[j][1]);  // solo escribo en pipes[i][1]
        if (j != (i - 1 + n) % n && !(i == start && j == i)) {
            close(pipes[j][0]);  // solo leo de mi predecesor o del padre si soy start
        }
    }

    close(parent_pipe[0]); // nunca leo del pipe padre

    int num;

    if (i == start) {
        // 1. Leo del padre
        read(pipes[i][0], &num, sizeof(int));
        printf("[Hijo %d] Recibí %d del padre\n", i, num);

        // 2. Envío al siguiente sin incrementar
        write(pipes[i][1], &num, sizeof(int));
        printf("[Hijo %d] Envié %d al hijo %d\n", i, num, (i + 1) % n);

        // 3. Leo la vuelta desde el predecesor
        read(pipes[(i - 1 + n) % n][0], &num, sizeof(int));
        printf("[Hijo %d] Recibí %d del hijo %d (vuelta)\n", i, num, (i - 1 + n) % n);

        // 4. Incremento final y envío al padre
        num++;
        printf("[Hijo %d] Incremento final a %d\n", i, num);

        write(parent_pipe[1], &num, sizeof(int));
        printf("[Hijo %d] Envié %d al padre\n", i, num);

    } else {
        // Resto de procesos
        read(pipes[(i - 1 + n) % n][0], &num, sizeof(int));
        printf("[Hijo %d] Recibí %d del hijo %d\n", i, num, (i - 1 + n) % n);

        num++;
        printf("[Hijo %d] Incremento a %d\n", i, num);

        write(pipes[i][1], &num, sizeof(int));
        printf("[Hijo %d] Envié %d al hijo %d\n", i, num, (i + 1) % n);
    }

    exit(0);
}
