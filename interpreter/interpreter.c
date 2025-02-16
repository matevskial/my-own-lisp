#include "interpreter.h"

#include <math.h>
#include <stdio.h>

static char *UNKNOWN_ERROR_MESSAGE = "Unknown error";
static char *ERR_INVALID_OPERATOR_MESSAGE = "Invalid Operator";
static char *ERR_DIV_ZERO_MESSAGE = "Division by zero";
static char *ERR_BAD_NUMERIC_VALUE_MESSAGE = "Bad number value";
static char *ERR_INCOMPATIBLE_TYPES_MESSAGE = "Incompatible types for operation";

lisp_value_t lisp_value_number_new(long value) {
    lisp_value_t lisp_value;
    lisp_value.error = NO_ERROR;
    lisp_value.value_type = VAL_NUMBER;
    lisp_value.value_number = value;
    lisp_value.value_decimal = 0;
    return lisp_value;
}

lisp_value_t lisp_value_decimal_new(double value) {
    lisp_value_t lisp_value;
    lisp_value.error = NO_ERROR;
    lisp_value.value_type = VAL_DECIMAL;
    lisp_value.value_decimal = value;
    lisp_value.value_number = 0;
    return lisp_value;
}

lisp_value_t lisp_value_error_new(lisp_error_type_t error) {
    lisp_value_t lisp_error;
    lisp_error.error = error;
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
        case ERR_INCOMPATIBLE_TYPES:
            return ERR_INCOMPATIBLE_TYPES_MESSAGE;
        default:
            return UNKNOWN_ERROR_MESSAGE;
    }
}

void print_lisp_value(lisp_value_t lisp_value) {
    if (is_lisp_value_error(lisp_value)) {
        printf("error: %s", get_lisp_value_error_message(lisp_value));
    } else {
        if (lisp_value.value_type == VAL_NUMBER) {
            printf("%ld", lisp_value.value_number);
        } else if (lisp_value.value_type == VAL_DECIMAL) {
            printf("%f", lisp_value.value_decimal);
        }
    }
}

lisp_value_t add_lisp_values(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1.value_number + value2.value_number);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1.value_decimal + value2.value_decimal);
    }
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1.value_number + value2.value_decimal);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1.value_decimal + (double) value2.value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t subtract_lisp_values(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1.value_number - value2.value_number);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1.value_decimal - value2.value_decimal);
    }
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1.value_number - value2.value_decimal);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1.value_decimal - (double) value2.value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t multiply_lisp_values(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1.value_number * value2.value_number);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1.value_decimal * value2.value_decimal);
    }
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1.value_number * value2.value_decimal);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1.value_decimal * (double) value2.value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t divide_lisp_values(lisp_value_t value1, lisp_value_t value2) {
    if (value2.value_type == VAL_NUMBER && value2.value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }
    if (value2.value_type == VAL_DECIMAL && value2.value_decimal== 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }

    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1.value_number / value2.value_number);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1.value_decimal / value2.value_decimal);
    }
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1.value_number / value2.value_decimal);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1.value_decimal / (double) value2.value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t division_remainder_lisp_value(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type != VAL_NUMBER || value2.value_type != VAL_NUMBER) {
        return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
    }
    if (value2.value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }

    return lisp_value_number_new(value1.value_number % value2.value_number);
}

lisp_value_t pow_lisp_value(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        return lisp_value_number_new((long) pow((double) value1.value_number, (double) value2.value_number));
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(pow(value1.value_decimal, value2.value_decimal));
    }
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(pow((double) value1.value_number, value2.value_decimal));
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(pow( value1.value_decimal, (double) value2.value_number));
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t min_lisp_value(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        long min = value1.value_number < value2.value_number ? value1.value_number : value2.value_number;
        return lisp_value_number_new(min);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        double min = value1.value_decimal < value2.value_decimal ? value1.value_decimal : value2.value_decimal;
        return lisp_value_decimal_new(min);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t max_lisp_value(lisp_value_t value1, lisp_value_t value2) {
    if (value1.value_type == VAL_NUMBER && value2.value_type == VAL_NUMBER) {
        long max = value1.value_number > value2.value_number ? value1.value_number : value2.value_number;
        return lisp_value_number_new(max);
    }
    if (value1.value_type == VAL_DECIMAL && value2.value_type == VAL_DECIMAL) {
        double max = value1.value_decimal > value2.value_decimal ? value1.value_decimal : value2.value_decimal;
        return lisp_value_decimal_new(max);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t negate_lisp_value(lisp_value_t value) {
    if (value.value_type == VAL_NUMBER) {
        return lisp_value_number_new(-value.value_number);
    }
    if (value.value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(-value.value_decimal);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}
