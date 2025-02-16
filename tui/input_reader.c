#include "input_reader.h"

#include "config.h"

#include <stdlib.h>

#if defined(_WINDOWS) || defined(READ_LINE_STDIN_WITH_STDIO)
#include <stdio.h>
#include <unistd.h>
#endif

#if defined(_UNIX_STYLE_OS)
#include <editline/readline.h>
#include <string.h>
#endif

size_t read_line_stdin_scanf(const char *prompt, char *buff, size_t size) {
    fputs(prompt, stdout);
    fflush(stdout);

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

size_t read_line_stdin_fgets(const char *prompt, char *buff, size_t size) {
    fputs(prompt, stdout);
    fflush(stdout);

    char* result = fgets(buff, size, stdin);
    if (result == NULL) {
        return 0;
    }
    if (feof(stdin) || ferror(stdin)) {
        if (ferror(stdin)) {
            fflush(stdin);
        }
        return 0;
    }

    // counting null-terminating character too
    size_t input_final_size = strlen(buff) + 1;

    // note: ignores additional input if stdin has more bytes of input than size, so reading is not continuing automatically for the remaining input
    if (input_final_size > 1 && buff[input_final_size - 2] != '\n') {
        int c;
        while ((c = fgetc(stdin)) != EOF && c != '\n') {
            if (ferror(stdin)) {
                fflush(stdin);
                break;
            }
        }
    }

    if (input_final_size > 1 && buff[input_final_size - 2] == '\n') {
        buff[input_final_size - 2] = '\0';
        input_final_size--;
    }

    return input_final_size;
}

#if defined(_WINDOWS) || defined(READ_LINE_STDIN_WITH_STDIO)
size_t read_line_stdin_internal(const char *prompt, char *buff, size_t size) {
    return read_line_stdin_fgets(prompt, buff, size);
}
#elif defined(_UNIX_STYLE_OS)
size_t read_line_stdin_internal(const char *prompt, char *buff, size_t size) {
    char *input = readline(prompt);
    if (input == NULL) {
        return 0;
    }

    // counting null-terminating character too
    size_t input_full_size = strlen(input) + 1;

    if (input_full_size > 1 && input[0]) {
        add_history(input);
    }

    size_t size_to_copy = input_full_size < size ? input_full_size : size;
    strncpy(buff, input, size_to_copy);
    buff[size_to_copy - 1] = '\0';

    free(input);
    return size_to_copy;
}
#else
size_t read_line_stdin_internal(const char *prompt, char *buff, size_t size) {
}
#endif

void add_history_internal(const char *buff) {
#if defined(_UNIX_STYLE_OS)
    add_history(buff);
#endif
}

size_t read_line_stdin(const char *prompt, char *buff, size_t size) {
    /* Should check with 2, not with 1 since fgets does not wait for user input if we pass size value 1.
     * For repl implementations, it would result in printing the prompt infinitely */
    if (size < 2) {
        return 0;
    }

    size_t line_size = read_line_stdin_internal(prompt, buff, size);
    if (line_size > 1) {
        add_history_internal(buff);
    }
    return line_size;
}
