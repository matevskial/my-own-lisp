#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "tui/input_reader.h"
#include "interpreter/interpreter.h"

/* constexpr(keyword since C23) used so that we don't get variably modified at scope compiler error
 * while using variable to store the buffer size
 */
static constexpr size_t input_buff_size = 2048;
static char input_buff[input_buff_size];
static char *prompt = "my-own-lisp> ";

static char* REPL_COMMAND_EXIT = "exit";
static char* REPL_COMMAND_PRINT_LISP_ENVIRONMENT = "print_lisp_environment";

int main(int argc, char **argv) {
    parser_initialize();

    lisp_environment_t* env = lisp_environment_new_root();
    bool env_setup_successful = lisp_environment_setup_builtin_functions(env);

    if (!env_setup_successful) {
        puts("Failed to initialize lisp evaluation environment. Probably out of memory.");
        exit(1);
    }

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            lisp_value_t* result = load_file(env, argv[i]);
            if (result == get_null_lisp_value()) {
                printf("Error encountered during loading file %s: null lisp value", argv[i]);
            } else if (result->value_type == VAL_ERR) {
                printf("Error encountered during loading file %s: %s\n", argv[i], result->error_message);
            }
            lisp_value_delete(result);
        }
    } else {
        puts("my-own-lisp version 0.0.1");
        puts("Press Ctrl-C to exit\n");

        while (1) {
            size_t size_read = read_line_stdin(prompt, input_buff, input_buff_size);

            if (size_read < 1) {
                puts("input error");
                exit(1);
            }

            if (size_read == 1) {
                continue;
            }

            if (!lisp_environment_exists(env, input_buff)) {
                if (strcmp(input_buff, REPL_COMMAND_EXIT) == 0) {
                    break;
                }
                if (strcmp(input_buff, REPL_COMMAND_PRINT_LISP_ENVIRONMENT) == 0) {
                    println_lisp_environment(env);
                    continue;
                }
            }

            mpc_result_t my_own_lisp_parse_result;
            if (parse("<stdin>", input_buff, &my_own_lisp_parse_result)) {
                lisp_value_t* lisp_value = parse_lisp_value(my_own_lisp_parse_result.output);
                // lisp_eval_result_t* eval_result = evaluate_root_lisp_value(lisp_value);
                lisp_eval_result_t* eval_result_1 = evaluate_root_lisp_value_destructive(env, lisp_value);
                // print_lisp_eval_result(eval_result);
                // putchar('\n');
                print_lisp_eval_result(eval_result_1);
                putchar('\n');
                // lisp_value_delete(lisp_value);
                // lisp_eval_result_delete(eval_result);
                // TODO: handle the case where eval_result_1 is NULL for some reason
                lisp_eval_result_delete(eval_result_1);
                mpc_ast_delete(my_own_lisp_parse_result.output);
            } else {
                mpc_err_print(my_own_lisp_parse_result.error);
                mpc_err_delete(my_own_lisp_parse_result.error);
            }
        }
    }

    parser_cleanup();
    lisp_environment_delete(env);
    return 0;
}
