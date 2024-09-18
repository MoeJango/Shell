#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>


void process(char* input, char* words[], int* word_count) {
    char *delim = " \t\n";  
    char *token;
    int count = 0;

    while ((token = strsep(&input, delim)) != NULL) {
        if (*token == '\0') {
            continue;
        }
        if (words[count] != NULL) {
            free(words[count]);
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
        return 2;
    }
    else if (word_count == 0) {
        return 3;
    }
    else {
        return 4;
    }
}

bool findPath(char program[], char* paths[], char** path, int numPaths) {
	if (paths[0] == NULL) {
		free(*path);
		*path = NULL;
		return false;
	}
	for (int i=0; i<numPaths; i++) {
		*path = realloc(*path, strlen(paths[i]) + strlen(program) + 1);
		strcpy(*path, paths[i]);
		strcat(*path, program);
		if (access(*path, X_OK) == 0) {
			return true;
		}
	}
	free(*path);
	*path = NULL;
	return false;
}


int main(char argc, char* argv[]) {
	const char error_message[30] = "An error has occurred\n";
	FILE* file;

	int mode;
	if (argc == 1) {
		mode = 0;
	}
	else if (argc == 2) {
		mode = 1;
		file = fopen(argv[1], "r");
		if (file == NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
	}
	else {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}

    char* input = NULL;
    size_t len;

    int pipefd1[2], pipefd2[2];
    ssize_t bytesRead;
    char buffer[1024];

    char** paths = malloc(sizeof(char*));
    paths[0] = malloc(1024);
	int numPaths = 1;
    strcpy(paths[0], "/bin/");

    char** words = NULL;
    int int_word_count = 0;
    int* word_count = &int_word_count;

    char prompt[] = "witsshell> ";

    do {
		if (mode == 0) {
        	printf("%s", prompt);
		}

		if (mode == 0) {
        	getline(&input, &len, stdin);
		}
		else {
			getline(&input, &len, file);
		}

		if (mode == 1) {
			if (feof(file)) {
				break;
			}
		}
		else if (feof(stdin)) {
			break;
		}

		if (input == NULL || strlen(input) == 0 || strcmp(input, "\n") == 0) {
    		continue;  
		}

		

        words = realloc(words, sizeof(char*) * (strlen(input)/2 + 2));

        for (int i = 0; i < strlen(input)/2 + 2; i++) {
            words[i] = NULL;
        }

        process(input, words, word_count);

		if (*word_count == 0) {
			continue;
		}

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
			if (*word_count == 1) {
				paths = realloc(paths, sizeof(char*));
				paths[0] = NULL;
			}
			else {
				paths = realloc(paths, sizeof(char*) * (*word_count-1));
				numPaths = *word_count-1;
				for (int i = 0; i < *word_count-1; i++) {
					paths[i] = malloc(strlen(words[i+1]) + 1);
					strcpy(paths[i], words[i+1]);
				}
			}
        }

        else if (check == 3) {
            continue;
        }

        else {
			char* exec_path = NULL;
			if (findPath(words[0], paths, &exec_path, numPaths)) {
				if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
					perror("");
				}

				pid_t pid = fork();
				if (pid < 0) {
					perror("");
				}
				else if (pid == 0) {
					char* args[*word_count+1];
					for (int i = 0; i < *word_count; i++) {
						args[i] = malloc(strlen(words[i]) + 1);
						strcpy(args[i], words[i]);
					}
					args[*word_count] = NULL;

					close(pipefd1[0]);
					close(pipefd2[0]);

					dup2(pipefd1[1], STDOUT_FILENO);
					dup2(pipefd2[1], STDERR_FILENO);

					close(pipefd1[1]);
					close(pipefd2[1]);

					execv(exec_path, args);

					perror("");
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
			else {
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
        }
    }
    while (1);
    exit(0);
}


