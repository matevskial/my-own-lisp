#include "parser.h"

// this is a language with bonus marks additions
// it is in a separate variable because some of the bonus mark probably won't be used in the language my-own-lisp
// it is left here for reference
static char *chapter_6_bonus_mark_language =
    "                                                                         \
        number          : /-?[0-9]+/ ;                                        \
        operator        : '+' | '-' | '*' | '/' | '%' | ;                     \
        expr            : <number> | /-?//[0-9]/'.'/[0-9]/ | /-?/'.'/[0-9]/ | '(' <operator> <expr>+ ')' ;             \
        my_own_lisp     : /^/ <operator> <expr>+ /$/ | /^[ab]+$/ ;    \
    ";

static char *my_own_lisp_language =
        "                                                                                                                 \
        number            : /-?[0-9]+/ ;                                                                                  \
        decimal           : /-?[0-9]*\\.[0-9]+/ ;                                                                         \
        boolean           : \"true\" | \"false\" ;                                                                        \
        string            : /\"(\\\\.|[^\"])*\"/ ;                                                                        \
        comment           : /;[^\\r\\n]*/ ;                                                                               \
        symbol            : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%^|]+/ ;                                                         \
        sexpr             : '(' <expr>* ')' ;                                                                             \
        qexpr             : '{' <expr>* '}' ;                                                                             \
        expr              : <decimal> | <number> | <boolean> | <string> | <comment> | <symbol> | <sexpr> | <qexpr> ;      \
        my_own_lisp       : /^/ <expr>* /$/ ;                                                                             \
    ";

static mpc_parser_t *number;
static mpc_parser_t *decimal;
static mpc_parser_t *boolean;
static mpc_parser_t *symbol;
static mpc_parser_t *sexpr;
static mpc_parser_t *expr;
static mpc_parser_t *qexpr;
static mpc_parser_t *string;
static mpc_parser_t *comment;
static mpc_parser_t *my_own_lisp;

void parser_initialize() {
    number = mpc_new("number");
    decimal = mpc_new("decimal");
    boolean = mpc_new("boolean");
    symbol = mpc_new("symbol");
    sexpr = mpc_new("sexpr");
    expr = mpc_new("expr");
    qexpr = mpc_new("qexpr");
    string = mpc_new("string");
    comment = mpc_new("comment");
    my_own_lisp = mpc_new("my_own_lisp");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        my_own_lisp_language,
        number,
        decimal,
        boolean,
        symbol,
        sexpr,
        expr,
        qexpr,
        string,
        comment,
        my_own_lisp);
}

void parser_cleanup() {
    mpc_cleanup(10,
                number,
                decimal,
                boolean,
                symbol,
                sexpr,
                expr,
                qexpr,
                string,
                comment,
                my_own_lisp);
}

int parse(const char* filename, const char* string, mpc_result_t* result) {
    return mpc_parse("<stdin>", string, my_own_lisp, result);
}

int parse_contents(const char* filename, mpc_result_t* result) {
    return mpc_parse_contents(filename, my_own_lisp, result);
}
