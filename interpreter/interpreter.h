#pragma once
#include <stddef.h>

typedef enum {
    VAL_ERR,
    VAL_NUMBER,
    VAL_DECIMAL,
    VAL_SYMBOL,
    VAL_SEXPR,
    VAL_ROOT,
    VAL_QEXPR,
    VAL_BUILTIN_FUN,
    VAL_USERDEFINED_FUN
} lisp_value_type_t;

// forward declarations
typedef struct lisp_value_t lisp_value_t;
typedef struct lisp_environment_t lisp_environment_t;

typedef struct lisp_value_userdefined_fun_t {
    lisp_value_t* formal_arguments;
    lisp_value_t* varargs_symbol;
    lisp_value_t* body;
    lisp_environment_t* local_env;
} lisp_value_userdefined_fun_t;

typedef struct lisp_value_t {
    lisp_value_type_t value_type;
    char* error_message;
    long value_number;
    double value_decimal;
    char* value_symbol;
    lisp_value_userdefined_fun_t* value_userdefined_fun;
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
    lisp_environment_t* parent_environment;
} lisp_environment_t;

lisp_value_t* lisp_value_number_new(long value);
lisp_value_t* lisp_value_decimal_new(double value);
lisp_value_t* lisp_value_symbol_new(char* value);
lisp_value_t* lisp_value_sexpr_new();
lisp_value_t* lisp_value_root_new();
lisp_value_t* lisp_value_qexpr_new();
lisp_value_t* lisp_value_builtin_fun_new(char* symbol);
lisp_value_t* lisp_value_userdefined_fun_new(lisp_environment_t* environment, lisp_value_t* formal_arguments, lisp_value_t* body);
lisp_value_t* lisp_value_error_new(char* error_message_template, ...);
lisp_value_t* lisp_value_copy(lisp_value_t* value);
lisp_value_t* get_null_lisp_value();
lisp_value_t* get_lisp_value_error_bad_numeric_value();
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
lisp_environment_t* lisp_environment_new_root();
lisp_environment_t* lisp_environment_new_with_parent(lisp_environment_t* env);
lisp_environment_t* lisp_environment_copy(lisp_environment_t* env);
lisp_value_t* lisp_environment_put_variables(lisp_environment_t* env, lisp_value_t* arguments, char* function_name);
void lisp_environment_delete(lisp_environment_t* env);
bool lisp_environment_set(lisp_environment_t* env, lisp_value_t* symbol, lisp_value_t* value);
lisp_value_t* lisp_environment_get(lisp_environment_t* env, lisp_value_t* symbol);
bool lisp_environment_exists(lisp_environment_t* env, char* symbol_str);
bool is_lisp_environment_null(lisp_environment_t* env);
bool lisp_environment_setup_builtin_functions(lisp_environment_t* env);
void println_lisp_environment(lisp_environment_t* env);
