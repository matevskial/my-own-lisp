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
        symbol            : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;                  \
        sexpr             : '(' <expr>* ')' ;                                                        \
        qexpr             : '{' <expr>* '}' ;                                                        \
        expr              : <symbol> | <decimal> | <number> | <sexpr> | <qexpr> ;                    \
        my_own_lisp       : /^/ <expr>* /$/ ;                                                        \
    ";

bool has_tag(mpc_ast_t * ast, char * tag) {
    return strstr(ast->tag, tag) != NULL;
}

bool is_root(mpc_ast_t * ast) {
    return strcmp(ast->tag, ">") == 0;
}


/* Assumes ast is not NULL
 * Will return get_null_lisp_value() if the lisp_value is not parsed completely
 *  * if the ast does not represent the language that this function parses
 *  * if the program is not able to allocate lisp_value_t* objects
 */
lisp_value_t* parse_lisp_value(mpc_ast_t* ast) {
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
    if (has_tag(ast, "symbol")) {
        return lisp_value_symbol_new(ast->contents);
    }

    lisp_value_t* lisp_value = get_null_lisp_value();
    if (is_root(ast)) {
        lisp_value = lisp_value_root_new();
    } else if (has_tag(ast, "sexpr")) {
        lisp_value = lisp_value_sexpr_new();
    } else if (has_tag(ast, "qexpr")) {
        lisp_value = lisp_value_qexpr_new();
    }

    if (is_lisp_value_null(lisp_value)) {
        return get_null_lisp_value();
    }

    int i = 1;
    while (has_tag(ast->children[i], "expr")) {
        lisp_value_t* current_parsed_lisp_value = parse_lisp_value(ast->children[i]);
        if (is_lisp_value_null(current_parsed_lisp_value)) {
            lisp_value_delete(lisp_value);
            return get_null_lisp_value();
        }
        bool ok = append_lisp_value(lisp_value, current_parsed_lisp_value);
        if (!ok) {
            lisp_value_delete(current_parsed_lisp_value);
            lisp_value_delete(lisp_value);
            return get_null_lisp_value();
        }
        i++;
    }

    return lisp_value;
}

int main(int argc, char** argv) {
    mpc_parser_t* number = mpc_new("number");
    mpc_parser_t* decimal = mpc_new("decimal");
    mpc_parser_t* symbol = mpc_new("symbol");
    mpc_parser_t* sexpr = mpc_new("sexpr");
    mpc_parser_t* expr = mpc_new("expr");
    mpc_parser_t* qexpr = mpc_new("qexpr");
    mpc_parser_t* my_own_lisp = mpc_new("my_own_lisp");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        my_own_lisp_language,
       number, decimal, symbol, sexpr, expr, qexpr, my_own_lisp
    );

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

        mpc_result_t my_own_lisp_parse_result;
        if (mpc_parse("<stdin>", input_buff, my_own_lisp, &my_own_lisp_parse_result)) {
            lisp_value_t* lisp_value = parse_lisp_value(my_own_lisp_parse_result.output);
            // lisp_eval_result_t* eval_result = evaluate_root_lisp_value(lisp_value);
            lisp_eval_result_t* eval_result_1 = evaluate_root_lisp_value_destructive(lisp_value);
            // print_lisp_eval_result(eval_result);
            // putchar('\n');
            print_lisp_eval_result(eval_result_1);
            putchar('\n');
            // lisp_value_delete(lisp_value);
            // lisp_eval_result_delete(eval_result);
            lisp_eval_result_delete(eval_result_1);
            mpc_ast_delete(my_own_lisp_parse_result.output);
        } else {
            mpc_err_print(my_own_lisp_parse_result.error);
            mpc_err_delete(my_own_lisp_parse_result.error);
        }
    }

    mpc_cleanup(7, number, decimal, symbol, sexpr, expr, qexpr, my_own_lisp);
    return 0;
}
