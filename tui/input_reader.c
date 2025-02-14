#if defined(_WIN32)
#include <stdio.h>
#include <unistd.h>
#endif

#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
#include <editline/readline.h>
#include <string.h>
#include <stdlib.h>
#endif

size_t read_line_stdin_scanf(char *buff, size_t size) {
    char *input = malloc(sizeof(char) * size);
    // scanf resizes buffer to arbitrary length - no protection from long lines
    int result = scanf("%s", input);
    if (result == 0 || result == EOF) {
        return 0;
    }

    // counting null-terminating character too
    size_t input_final_size = strlen(input) + 1;
    size_t size_to_copy = input_final_size < size ? input_final_size : size;
    strncpy(buff, input, size_to_copy);
    buff[size_to_copy - 1] = '\0';

    free(input);
    return size_to_copy;
}

size_t read_line_stdin(const char *prompt, char *buff, size_t size) {
    if (size < 2) {
        return 0;
    }

#if defined(_WIN32)
    fputs(prompt, stdout);
    fflush(stdout);

    return read_line_stdin_scanf(buff, size);
#elif defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
    char *input = readline(prompt);
    if (input == NULL) {
        return 0;
    }

    // counting null-terminating character too
    size_t input_full_size = strlen(input) + 1;

    add_history(input);

    size_t size_to_copy = input_full_size < size ? input_full_size : size;
    strncpy(buff, input, size_to_copy);
    buff[size_to_copy - 1] = '\0';

    free(input);
    return size_to_copy;
#else
    return 0
#endif
}
