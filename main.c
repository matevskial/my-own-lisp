#include <stdio.h>
#include "tui/input_reader.h"

/* constexpr(keyword since C23) used so that we don't get variably modified at scope compiler error
 * while using variable to store the buffer size
 */
static constexpr size_t input_buff_size = 2048;
static char input_buff[input_buff_size];
static char *prompt = "my-own-lisp> ";

int main(int argc, char** argv) {
    puts("my-own-lisp version 0.0.1");
    puts("Press Ctrl-C to exit\n");

    while (1) {
        read_line_stdin(prompt, input_buff, input_buff_size);

        printf("%s\n", input_buff);
    }

    return 0;
}
