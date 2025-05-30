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
    // Cerrar todos los pipes que no usa
    for (int j = 0; j < n; j++) {
        if (j != i) close(pipes[j][1]); // No escribo en pipes[j] si no soy yo
        if (j != (i - 1 + n) % n && !(i == start && j == i)) close(pipes[j][0]); // Solo leo del anterior (o del padre si soy el start)
    }
    close(parent_pipe[0]); // No leo del padre

    int num;

    // Leer
    if (i == start) {
        // Si soy el proceso que arranca, leo lo que me mandó el padre
        read(pipes[i][0], &num, sizeof(int));
        printf("[Hijo %d] Recibí %d del padre\n", i, num);
    } else {
        read(pipes[(i - 1 + n) % n][0], &num, sizeof(int));
        printf("[Hijo %d] Recibí %d del hijo %d\n", i, num, (i - 1 + n) % n);
    }

    // Incrementar
    num++;
    printf("[Hijo %d] Incremento a %d\n", i, num);

    // Escribir
    if (i == start) {
        write(parent_pipe[1], &num, sizeof(int));
        printf("[Hijo %d] Envié %d al padre\n", i, num);
    } else {
        write(pipes[i][1], &num, sizeof(int));
        printf("[Hijo %d] Envié %d al hijo %d\n", i, num, (i + 1) % n);
    }

    exit(0);
}


