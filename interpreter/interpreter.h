#pragma once
#include <stddef.h>

static char* ERR_INVALID_OPERATOR_MESSAGE = "Invalid Operator";
static char* ERR_DIV_ZERO_MESSAGE = "Division by zero";
static char* ERR_BAD_NUMERIC_VALUE_MESSAGE = "Bad numeric value";
static char* ERR_INCOMPATIBLE_TYPES_MESSAGE = "Incompatible types for operation";
static char* ERR_UNBOUND_SYMBOL_MESSAGE = "Unbound symbol";
static char* ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE = "Incompatible type for argument %d of %s: expected %s, got %s";
static char* ERR_BUILTIN_DEF_INVALID_VALUE_COUNT_MESSAGE_TEMPLATE = "Invalid number of values for defining variables: expected %d, got %d";
static char* ERR_BUILTIN_DEF_INVALID_TYPE_FOR_VARIABLE_NAME_MESSAGE_TEMPLATE = "Invalid type for variable name: expected %s, got %s";
static char* ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE = "Invalid number of arguments for %s: expected: %d, got %d";
static char* ERR_AT_LEAST_ONE_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE = "Expected at least one argument for %s";
static char* ERR_NOT_ALLOWED_TO_REDEFINE_BUILTIN_FUN_MESSAGE_TEMPLATE = "Builtin %s not allowed to be redefined";

typedef enum {
    VAL_ERR,
    VAL_NUMBER,
    VAL_DECIMAL,
    VAL_SYMBOL,
    VAL_SEXPR,
    VAL_ROOT,
    VAL_QEXPR,
    VAL_BUILTIN_FUN
} lisp_value_type_t;

typedef struct lisp_value_t {
    lisp_value_type_t value_type;
    char* error_message;
    long value_number;
    double value_decimal;
    char* value_symbol;
    long count;
    struct lisp_value_t** values;
} lisp_value_t;

typedef struct lsp_eval_result_t {
    char* error;
    lisp_value_t* value;
} lisp_eval_result_t;

typedef struct lisp_environment_t {
    size_t count;
    char** symbols;
    lisp_value_t** values;
} lisp_environment_t;

lisp_value_t* lisp_value_number_new(long value);
lisp_value_t* lisp_value_decimal_new(double value);
lisp_value_t* lisp_value_symbol_new(char* value);
lisp_value_t* lisp_value_sexpr_new();
lisp_value_t* lisp_value_root_new();
lisp_value_t* lisp_value_qexpr_new();
lisp_value_t* lisp_value_builtin_fun_new(char* symbol);
lisp_value_t* lisp_value_error_new(char* error_message_template, ...);
lisp_value_t* lisp_value_copy(lisp_value_t* value);
lisp_value_t* get_null_lisp_value();
bool append_lisp_value(lisp_value_t* value, lisp_value_t* child_to_append);
void lisp_value_set_child(lisp_value_t* value, int index, lisp_value_t* child);
lisp_value_t* lisp_value_pop_child(lisp_value_t * value, int index);
void lisp_value_delete(lisp_value_t *lisp_value);

lisp_eval_result_t* lisp_eval_result_new(lisp_value_t* value);
void lisp_eval_result_delete(lisp_eval_result_t* lisp_eval_result);

bool is_lisp_value_error(lisp_value_t* value);
bool is_lisp_value_null(lisp_value_t* value);

void print_lisp_value(lisp_value_t* value);
void print_lisp_eval_result(lisp_eval_result_t* lisp_eval_result);

lisp_value_t* add_lisp_values(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* subtract_lisp_values(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* multiply_lisp_values(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* divide_lisp_values(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* division_remainder_lisp_value(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* pow_lisp_value(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* min_lisp_value(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* max_lisp_value(lisp_value_t* value1, lisp_value_t* value2);
lisp_value_t* negate_lisp_value(lisp_value_t* value);

lisp_eval_result_t* evaluate_root_lisp_value(lisp_value_t* value);
lisp_eval_result_t* evaluate_root_lisp_value_destructive(lisp_environment_t *env, lisp_value_t* value);
lisp_value_t* evaluate_lisp_value_destructive(lisp_environment_t* env, lisp_value_t* value);

lisp_environment_t* lisp_environment_new();
void lisp_environment_delete(lisp_environment_t* env);
bool lisp_environment_set(lisp_environment_t* env, lisp_value_t* symbol, lisp_value_t* value);
lisp_value_t* lisp_environment_get(lisp_environment_t* env, lisp_value_t* symbol);
bool lisp_environment_exists(lisp_environment_t* env, char* symbol_str);
bool is_lisp_environment_null(lisp_environment_t* env);
bool lisp_environment_setup_builtin_functions(lisp_environment_t* env);
void println_lisp_environment(lisp_environment_t* env);
