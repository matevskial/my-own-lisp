#include <stdio.h>
#include <stdlib.h>

#include "tui/input_reader.h"
#include "mpc/mpc.h"

/* constexpr(keyword since C23) used so that we don't get variably modified at scope compiler error
 * while using variable to store the buffer size
 */
static constexpr size_t input_buff_size = 2048;
static char input_buff[input_buff_size];
static char *prompt = "my-own-lisp> ";

static char *my_own_lisp_language =
    "                                                                         \
        number          : /-?[0-9]+/ ;                                        \
        operator        : '+' | '-' | '*' | '/' | ;                           \
        expr            : <number> | '(' <operator> <expr>+ ')' ;             \
        my_own_lisp     : /^/ <operator> <expr>+ /$/ ;                        \
    ";

int main(int argc, char** argv) {
    mpc_parser_t* number = mpc_new("number");
    mpc_parser_t* operator = mpc_new("operator");
    mpc_parser_t* expr = mpc_new("expr");
    mpc_parser_t* my_own_lisp = mpc_new("my_own_lisp");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        my_own_lisp_language,
       number, operator, expr, my_own_lisp
    );

    puts("my-own-lisp version 0.0.1");
    puts("Press Ctrl-C to exit\n");

    while (1) {
        size_t size_read = read_line_stdin(prompt, input_buff, input_buff_size);

        if (size_read < 1) {
            puts("input error");
            exit(1);
        }

        mpc_result_t my_own_lisp_parse_result;
        if (mpc_parse("<stdin>", input_buff, my_own_lisp, &my_own_lisp_parse_result)) {
            mpc_ast_print(my_own_lisp_parse_result.output);
            mpc_ast_delete(my_own_lisp_parse_result.output);
        } else {
            mpc_err_print(my_own_lisp_parse_result.error);
            mpc_err_delete(my_own_lisp_parse_result.error);
        }

        if (size_read > 1) {
            printf("echo back: %s\n", input_buff);
        }
    }

    mpc_cleanup(4, number, operator, expr, my_own_lisp);
    return 0;
}
