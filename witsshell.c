#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


void process(char* input, char* words[], int* word_count) {
    char *delim = " \t\n";  
    char *token;
    int count = 0;

    while ((token = strsep(&input, delim)) != NULL) {
        if (*token == '\0') {
            continue;
        }
        
        words[count] = malloc(strlen(token) + 1);
        if (words[count] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(words[count], token);
        count++;
    }

    *word_count = count; 
}


int valid(char* words[], int word_count) {    // -1 if invalid, 0 if exit, 1 if cd, 2 if path, , 3 if empty, 4 if command
    if (strcmp(words[0], "exit") == 0) {
        if (word_count == 1) {
            return 0;
        }
        else {
            return -1;
        }
    }
    else if (strcmp(words[0], "cd") == 0) {
        if (word_count == 2) {
            return 1;
        }
        else {
            return -1;
        }
    }
    else if (strcmp(words[0], "path") == 0) {
        for (int i = 1; i < word_count; i++) {
            if (!(words[i][0] == '/' && words[i][strlen(words[i]) - 1] == '/')) {
                return -1;
            }
        }
        return 2;
    }
    else if (word_count == 0) {
        return 3;
    }
    else {
        return 4;
    }
}


int main() {
    char* input = NULL;
    size_t len;

    int pipefd1[2], pipefd2[2];
    ssize_t bytesRead;
    char buffer[1024];

    char** paths = malloc(sizeof(char*));
    paths[0] = malloc(1024);
    strcpy(paths[0], "/bin/");

    char prompt[] = "witsshell> ";

    const char error_message[30] = "An error has occurred\n";

    do {
        printf("%s", prompt);
        getline(&input, &len, stdin);

        char* words[(int)strlen(input)/2 + 2];
        int* word_count;

        process(input, words, word_count);

        int check = valid(words, *word_count);

        if (check == -1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

        else if (check == 0) {
            break;
        }

        else if (check == 1) {
            if (chdir(words[1]) == -1) {
                perror("");
            }
        }

        else if (check == 2) {
            paths = realloc(paths, sizeof(char*) * (*word_count-1));
            for (int i = 0; i < *word_count-1; i++) {
                paths[i] = malloc(strlen(words[i]) + 1);
                strcpy(paths[i], words[i]);
            }
        }

        else if (check == 3) {
            continue;
        }

        else {
            if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
                perror("");
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("");
            }
            else if (pid == 0) {
                char* args[*word_count];
                for (int i = 0; i < *word_count; i++) {
                    args[i] = malloc(strlen(words[i]) + 1);
                    strcpy(args[i], words[i]);
                }
                close(pipefd1[0]);
                close(pipefd2[0]);

                dup2(pipefd1[1], STDOUT_FILENO);
                dup2(pipefd2[1], STDERR_FILENO);

                close(pipefd1[1]);
                close(pipefd2[1]);

                execv(strcat(paths[0], words[0]), args);
                exit(0);
            }
            else {

                close(pipefd1[1]);
                close(pipefd2[1]);

                while ((bytesRead = read(pipefd1[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    printf("%s", buffer);
                }

                while ((bytesRead = read(pipefd2[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    write(STDERR_FILENO, buffer, strlen(buffer));
                }

                close(pipefd1[0]);
                close(pipefd2[0]);

                wait(NULL);
            }
        }
    }
    while (strcmp(input, "exit\n") != 0);
    exit(0);
}


