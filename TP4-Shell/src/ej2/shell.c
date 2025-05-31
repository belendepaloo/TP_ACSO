#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <wordexp.h>


#define MAX_COMMANDS 200
#define READ_END 0
#define WRITE_END 1

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        printf("Shell> ");
        fflush(stdout); // Asegura que se muestre el prompt

        // Leer línea de comando
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break; // EOF (Ctrl+D)
        }

        // Eliminar el salto de línea al final
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Tokenizar los comandos por "|"
        char *token = strtok(command, "|");
        while (token != NULL) 
        {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        // Crear pipes
        int pipes[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // Crear procesos para cada comando
        for (int i = 0; i < command_count; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // Redireccionar entrada si no es el primer comando
                if (i > 0) {
                    dup2(pipes[i - 1][READ_END], STDIN_FILENO);
                }

                // Redireccionar salida si no es el último comando
                if (i < command_count - 1) {
                    dup2(pipes[i][WRITE_END], STDOUT_FILENO);
                }

                // Cerrar todos los extremos de pipes
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipes[j][READ_END]);
                    close(pipes[j][WRITE_END]);
                }

                wordexp_t p;
                if (wordexp(commands[i], &p, 0) != 0) {
                    perror("wordexp");
                    exit(EXIT_FAILURE);
                }
                execvp(p.we_wordv[0], p.we_wordv);
                perror("execvp"); // Solo se ejecuta si execvp falla
                wordfree(&p);
                exit(EXIT_FAILURE);
            }
        }

        // En el padre: cerrar todos los pipes
        for (int i = 0; i < command_count - 1; i++) {
            close(pipes[i][READ_END]);
            close(pipes[i][WRITE_END]);
        }

        // Esperar a todos los hijos
        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }

        // Resetear para la siguiente línea de comandos
        command_count = 0;
    }

    return 0;
}
