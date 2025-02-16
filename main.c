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

// this is a language with bonus marks additions
// it is in a separate variable because some of the bonus mark probably won't be used in the language my-own-lisp
static char *chapter_6_bonus_mark_language =
    "                                                                         \
        number          : /-?[0-9]+/ ;                                        \
        operator        : '+' | '-' | '*' | '/' | '%' | ;                     \
        expr            : <number> | /-?//[0-9]/'.'/[0-9]/ | /-?/'.'/[0-9]/ | '(' <operator> <expr>+ ')' ;             \
        my_own_lisp     : /^/ <operator> <expr>+ /$/ | /^[ab]+$/ ;    \
    ";

static char *my_own_lisp_language =
    "                                                                                            \
        number          : /-?[0-9]+/ ;                                                           \
        decimal_number  : /-?[0-9]*\\.[0-9]+/ ;                                                   \
        operator        : '+' | '-' | '*' | '/' | '%' ;                                          \
        expr            : <decimal_number> | <number> | '(' <operator> <expr>+ ')' ;             \
        my_own_lisp     : /^/ <operator> <expr>+ /$/ ;                                           \
    ";

bool has_tag(mpc_ast_t * ast, char * tag) {
    return strstr(ast->tag, tag) != NULL;
}

long execute_operation(mpc_ast_t * operator, long first_operand, long second_operand) {
    if (strcmp(operator->contents, "+") == 0) {
        return first_operand + second_operand;
    }
    if (strcmp(operator->contents, "-") == 0) {
        return first_operand - second_operand;
    }
    if (strcmp(operator->contents, "*") == 0) {
        return first_operand * second_operand;
    }
    if (strcmp(operator->contents, "/") == 0) {
        return first_operand / second_operand;
    }
    return 0;
}

/* Assumes ast is not NULL */
long eval(mpc_ast_t *ast) {
    if (has_tag(ast, "number")) {
        return atoi(ast->contents);
    }
    if (has_tag(ast, "decimal_number")) {
        return (long) atof(ast->contents);
    }

    mpc_ast_t *operator = ast->children[1];
    long first_expr_value = eval(ast->children[2]);
    long result = first_expr_value;

    int i = 3;
    while (has_tag(ast->children[i], "expr")) {
        long current_expr_value = eval(ast->children[i]);
        result = execute_operation(operator, result, current_expr_value);
        i++;
    }

    return result;
}

int main(int argc, char** argv) {
    mpc_parser_t* number = mpc_new("number");
    mpc_parser_t* decimal_number = mpc_new("decimal_number");
    mpc_parser_t* operator = mpc_new("operator");
    mpc_parser_t* expr = mpc_new("expr");
    mpc_parser_t* my_own_lisp = mpc_new("my_own_lisp");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        my_own_lisp_language,
       number, decimal_number, operator, expr, my_own_lisp
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
            long result = eval(my_own_lisp_parse_result.output);
            printf("%ld\n", result);
            mpc_ast_delete(my_own_lisp_parse_result.output);
        } else {
            mpc_err_print(my_own_lisp_parse_result.error);
            mpc_err_delete(my_own_lisp_parse_result.error);
        }
    }

    mpc_cleanup(4, number, operator, expr, my_own_lisp);
    return 0;
}
