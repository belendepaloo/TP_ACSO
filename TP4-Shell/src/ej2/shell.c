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
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        command[strcspn(command, "\n")] = '\0';
        char *token = strtok(command, "|");

        
        if (!strcmp(command, "exit")) {
            break;
        }
        while (token != NULL) 
        {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        int pipes[MAX_COMMANDS - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < command_count; i++) {
            int pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid) {
                if (i > 0) {
                    dup2(pipes[i - 1][READ_END], STDIN_FILENO);
                }

                if (i < command_count - 1) {
                    dup2(pipes[i][WRITE_END], STDOUT_FILENO);
                }

                for (int j = 0; j < command_count - 1; j++) {
                    close(pipes[j][READ_END]);
                    close(pipes[j][WRITE_END]);
                }

                wordexp_t p;
                if (wordexp(commands[i], &p, 0) != 0) {
                    perror("wordexp");
                    wordfree(&p); 
                    exit(EXIT_FAILURE);
                }
                execvp(p.we_wordv[0], p.we_wordv);
                perror("execvp");
                wordfree(&p);
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < command_count - 1; i++) {
            close(pipes[i][READ_END]);
            close(pipes[i][WRITE_END]);
        }

        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }
        command_count = 0;
    }
    return 0;
}
