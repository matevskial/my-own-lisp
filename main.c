#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

#include "tui/input_reader.h"
#include "interpreter/interpreter.h"

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
    "                                                                                                \
        number            : /-?[0-9]+/ ;                                                             \
        decimal           : /-?[0-9]*\\.[0-9]+/ ;                                                    \
        operator          : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;                  \
        expr              : <decimal> | <number> | '(' <operator> <expr>+ ')' ;                      \
        my_own_lisp       : /^/ <operator> <expr>+ /$/ ;                                             \
    ";

bool has_tag(mpc_ast_t * ast, char * tag) {
    return strstr(ast->tag, tag) != NULL;
}

lisp_value_t execute_operation(mpc_ast_t* operation, lisp_value_t first_operand, lisp_value_t second_operand) {
    if (is_lisp_value_error(first_operand)) {
        return first_operand;
    }
    if (is_lisp_value_error(second_operand)) {
        return second_operand;
    }

    if (strcmp(operation->contents, "+") == 0) {
        return add_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "-") == 0) {
        return subtract_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "*") == 0) {
        return multiply_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "/") == 0) {
        return divide_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "%") == 0) {
        return division_remainder_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "^") == 0) {
        return pow_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation->contents, "min") == 0) {
        lisp_value_t min = min_lisp_value(first_operand, second_operand);
        return min;
    }
    if (strcmp(operation->contents, "max") == 0) {
        lisp_value_t max = max_lisp_value(first_operand, second_operand);
        return max;
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR);
}

lisp_value_t execute_numeric_unary_operation(mpc_ast_t* operation, lisp_value_t operand) {
    if (is_lisp_value_error(operand)) {
        return operand;
    }
    if (strcmp(operation->contents, "-") == 0) {
        return negate_lisp_value(operand);
    }
    return operand;
}

/* Assumes ast is not NULL */
lisp_value_t eval(mpc_ast_t *ast) {
    if (has_tag(ast, "number")) {
        errno = 0;
        long number = strtol(ast->contents, NULL, 10);
        if (errno != 0) {
            return lisp_value_error_new(ERR_BAD_NUMERIC_VALUE);
        }
        return lisp_value_number_new(number);
    }
    if (has_tag(ast, "decimal")) {
        errno = 0;
        double number_decimal = strtod(ast->contents, NULL);
        if (errno != 0) {
            return lisp_value_error_new(ERR_BAD_NUMERIC_VALUE);
        }
        return lisp_value_decimal_new(number_decimal);
    }

    mpc_ast_t *operator = ast->children[1];
    lisp_value_t first_expr_value = eval(ast->children[2]);
    lisp_value_t result = execute_numeric_unary_operation(operator, first_expr_value);

    int i = 3;
    while (has_tag(ast->children[i], "expr")) {
        lisp_value_t current_expr_value = eval(ast->children[i]);
        result = execute_operation(operator, result, current_expr_value);
        i++;
    }

    return result;
}

int main(int argc, char** argv) {
    mpc_parser_t* number = mpc_new("number");
    mpc_parser_t* decimal = mpc_new("decimal");
    mpc_parser_t* operator = mpc_new("operator");
    mpc_parser_t* expr = mpc_new("expr");
    mpc_parser_t* my_own_lisp = mpc_new("my_own_lisp");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        my_own_lisp_language,
       number, decimal, operator, expr, my_own_lisp
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
            lisp_value_t result = eval(my_own_lisp_parse_result.output);
            print_lisp_value(result);
            putchar('\n');
            mpc_ast_delete(my_own_lisp_parse_result.output);
        } else {
            mpc_err_print(my_own_lisp_parse_result.error);
            mpc_err_delete(my_own_lisp_parse_result.error);
        }
    }

    mpc_cleanup(4, number, operator, expr, my_own_lisp);
    return 0;
}
