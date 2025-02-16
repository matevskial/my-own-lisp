#include "interpreter.h"

#include <stdio.h>

static char *UNKNOWN_ERROR_MESSAGE = "Unknown error";
static char *ERR_INVALID_OPERATOR_MESSAGE = "Invalid Operator";
static char *ERR_DIV_ZERO_MESSAGE = "Division by zero";
static char *ERR_BAD_NUMERIC_VALUE_MESSAGE = "Bad number value";

lisp_value_t lisp_value_new(long value) {
    lisp_value_t lisp_value;
    lisp_value.error = NO_ERROR;
    lisp_value.value = value;
    return lisp_value;
}

lisp_value_t lisp_value_error_new(lisp_error_type_t error) {
    lisp_value_t lisp_error;
    lisp_error.error = error;
    lisp_error.value = 0;
    return lisp_error;
}

bool is_lisp_value_error(lisp_value_t lisp_value) {
    return !lisp_value.error == NO_ERROR;
}

char *get_lisp_value_error_message(lisp_value_t lisp_value) {
    switch (lisp_value.error) {
        case ERR_INVALID_OPERATOR:
            return ERR_INVALID_OPERATOR_MESSAGE;
        case ERR_DIV_ZERO:
            return ERR_DIV_ZERO_MESSAGE;
        case ERR_BAD_NUMERIC_VALUE:
            return ERR_BAD_NUMERIC_VALUE_MESSAGE;
        default:
            return UNKNOWN_ERROR_MESSAGE;
    }
}

void print_lisp_value(lisp_value_t lisp_value) {
    if (is_lisp_value_error(lisp_value)) {
        printf("error: %s", get_lisp_value_error_message(lisp_value));
    } else {
        printf("%ld", lisp_value.value);
    }
}
