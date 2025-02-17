#pragma once

typedef enum {
    NO_ERROR,
    ERR_INVALID_OPERATOR,
    ERR_DIV_ZERO,
    ERR_BAD_NUMERIC_VALUE,
    ERR_INCOMPATIBLE_TYPES,
    ERR_BAD_SEXPR
} lisp_error_type_t;

typedef enum {
    VAL_NUMBER,
    VAL_DECIMAL,
    VAL_SYMBOL,
    VAL_SEXPR,
    VAL_ROOT
} lisp_value_type_t;

typedef struct lisp_value_t {
    lisp_error_type_t error;
    lisp_value_type_t value_type;
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

lisp_value_t* lisp_value_number_new(long value);
lisp_value_t* lisp_value_decimal_new(double value);
lisp_value_t* lisp_value_symbol_new(char* value);
lisp_value_t* lisp_value_sexpr_new();
lisp_value_t* lisp_value_root_new();
lisp_value_t* lisp_value_error_new(lisp_error_type_t error);
lisp_value_t* get_null_lisp_value();
lisp_value_t* lisp_value_set_child(lisp_value_t* value, int index, lisp_value_t* child);
lisp_value_t* lisp_value_pop_child(lisp_value_t * value, int index);
void lisp_value_delete(lisp_value_t *lisp_value);

lisp_eval_result_t* lisp_eval_result_new(lisp_value_t* value);
void lisp_eval_result_delete(lisp_eval_result_t* lisp_eval_result);

bool append_lisp_value(lisp_value_t* sexpr, lisp_value_t* value);

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
lisp_eval_result_t* evaluate_root_lisp_value_destructive(lisp_value_t* value);
