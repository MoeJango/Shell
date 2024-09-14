#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int MAX_WORDS = 100;


void split_string(char *str, char *words[], int *word_count) {
    char *delim = " \t\n";  // Delimiters: space, tab, newline
    char *token;
    int count = 0;

    while ((token = strsep(&str, delim)) != NULL) {
        // Skip empty tokens
        if (*token == '\0') {
            continue;
        }
        // Allocate memory for each word
        words[count] = malloc(strlen(token) + 1);
        if (words[count] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(words[count], token);
        count++;
        if (count >= MAX_WORDS) {
            fprintf(stderr, "Maximum word limit reached\n");
            break;
        }
    }

    *word_count = count;  // Update the word count
}


int main() {
    char *words[MAX_WORDS];
    int word_count;
    char* str = malloc(1024);
    
    strcpy(str, "Hello Mao   c o w dos      sabtos              World       !\n");
    printf("%s", str);
    split_string(str, words, &word_count);
    for (int i = 0; i < word_count; i++) {
        printf("%s\n", words[i]);
        free(words[i]);
    }
}


