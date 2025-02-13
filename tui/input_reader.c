#if defined(_WIN32)
#include <stdio.h>
#include <unistd.h>
#endif

#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
#include <editline/readline.h>
#include <string.h>
#include <stdlib.h>
#endif

char *read_line_stdin(const char *prompt, char *buff, size_t size) {
#ifdef defined(_WIN32)
    fputs(prompt, stdout);
    char *input = malloc(sizeof(char) * size);
    scanf("%s", input);
    size_t input_full_size = strlen(input) + 1;
    size_t len_to_copy = input_full_size < size ? input_full_size : size;
    if (len_to_copy > 0) {
        len_to_copy--;
    }
    strncpy(buff, input, len_to_copy);
    buff[len_to_copy] = '\0';
    free(input);
    return buff;
#elif defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
    char *input = readline(prompt);
    size_t input_full_size = strlen(input) + 1;

    add_history(input);

    size_t len_to_copy = input_full_size < size ? input_full_size : size;
    if (len_to_copy > 0) {
        len_to_copy--;
    }
    strncpy(buff, input, len_to_copy);
    buff[len_to_copy] = '\0';

    free(input);
    return buff;
#else
    return buff;
#endif
}
