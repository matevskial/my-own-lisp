#pragma once

typedef enum {
    NO_ERROR,
    ERR_INVALID_OPERATOR,
    ERR_DIV_ZERO,
    ERR_BAD_NUMERIC_VALUE
} lisp_error_type_t;

typedef struct lisp_value_t {
    lisp_error_type_t error;
    long value;
} lisp_value_t;

lisp_value_t lisp_value_new(long value);

lisp_value_t lisp_value_error_new(lisp_error_type_t error);

bool is_lisp_value_error(lisp_value_t value);

void print_lisp_value(lisp_value_t value);
