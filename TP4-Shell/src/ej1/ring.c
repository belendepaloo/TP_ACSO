#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]);

int main(int argc, char **argv) {
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4) {
		printf("Uso: anillo <n> <c> <s> \n");
		exit(1);
	}

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

	int pipes[n][2];  // Cada pipe[i] es entre i y (i+1)%n

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

	// Crear hijos
	for (int i = 0; i < n; i++) {
		pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(1);
		} else if (pid == 0) {
			child_process(i, n, start, pipes, parent_pipe);
		}
	}

	// Padre escribe valor inicial al proceso 'start'
	write(pipes[start][1], &buffer[0], sizeof(int));

	// Cierra extremos no usados
	for (int i = 0; i < n; i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
	close(parent_pipe[1]);  // Solo va a leer del pipe padre

	// Leer resultado final
	int resultado_final;
	read(parent_pipe[0], &resultado_final, sizeof(int));
	close(parent_pipe[0]);

	// Esperar hijos
	for (int i = 0; i < n; i++) {
		wait(&status);
	}

	printf("Resultado final: %d\n", resultado_final);
	return 0;
}

void child_process(int i, int n, int start, int pipes[][2], int parent_pipe[2]) {
	// Cerrar los pipes que no uso
	for (int j = 0; j < n; j++) {
		if (j != (i - 1 + n) % n) close(pipes[j][0]);  // solo leo del anterior
		if (j != i) close(pipes[j][1]);  // solo escribo al siguiente
	}
	if (i != start) close(pipes[start][0]);  // solo el start lee del padre
	close(parent_pipe[0]);  // nunca leo del padre
	if ((i + 1) % n != start) close(parent_pipe[1]);  // solo el último escribe al padre

	int val;

	if (i == start) {
		// Leo valor inicial del padre
		read(pipes[i][0], &val, sizeof(int));
		// Lo paso al siguiente
		write(pipes[i][1], &val, sizeof(int));
		// Leo valor final desde el anterior
		read(pipes[(i - 1 + n) % n][0], &val, sizeof(int));
		val++;
		// Enviar al padre
		write(parent_pipe[1], &val, sizeof(int));
	} else {
		read(pipes[(i - 1 + n) % n][0], &val, sizeof(int));
		val++;
		write(pipes[i][1], &val, sizeof(int));
	}

	exit(0);
}
