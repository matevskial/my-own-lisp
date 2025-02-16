#pragma once

typedef enum {
    NO_ERROR,
    ERR_INVALID_OPERATOR,
    ERR_DIV_ZERO,
    ERR_BAD_NUMERIC_VALUE,
    ERR_INCOMPATIBLE_TYPES
} lisp_error_type_t;

typedef enum {
    VAL_NUMBER,
    VAL_DECIMAL
} lisp_value_type_t;

typedef struct lisp_value_t {
    lisp_error_type_t error;
    lisp_value_type_t value_type;
    long value_number;
    double value_decimal;
} lisp_value_t;

lisp_value_t lisp_value_number_new(long value);
lisp_value_t lisp_value_decimal_new(double value);
lisp_value_t lisp_value_error_new(lisp_error_type_t error);

bool is_lisp_value_error(lisp_value_t value);

void print_lisp_value(lisp_value_t value);

lisp_value_t add_lisp_values(lisp_value_t value1, lisp_value_t value2);
lisp_value_t subtract_lisp_values(lisp_value_t value1, lisp_value_t value2);
lisp_value_t multiply_lisp_values(lisp_value_t value1, lisp_value_t value2);
lisp_value_t divide_lisp_values(lisp_value_t value1, lisp_value_t value2);
lisp_value_t division_remainder_lisp_value(lisp_value_t value1, lisp_value_t value2);
lisp_value_t pow_lisp_value(lisp_value_t value1, lisp_value_t value2);
lisp_value_t min_lisp_value(lisp_value_t value1, lisp_value_t value2);
lisp_value_t max_lisp_value(lisp_value_t value1, lisp_value_t value2);
lisp_value_t negate_lisp_value(lisp_value_t value);
