#include "interpreter.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *UNKNOWN_ERROR_MESSAGE = "Unknown error";
static char *ERR_INVALID_OPERATOR_MESSAGE = "Invalid Operator";
static char *ERR_DIV_ZERO_MESSAGE = "Division by zero";
static char *ERR_BAD_NUMERIC_VALUE_MESSAGE = "Bad number value";
static char *ERR_INCOMPATIBLE_TYPES_MESSAGE = "Incompatible types for operation";
static char *ERR_BAD_SEXPR_MESSAGE = "Bad S-expression";

typedef lisp_value_t lisp_value_t;

static lisp_value_t null_lisp_value = {
    .error = 0,
    .value_type = 0,
    .value_number = 0,
    .value_decimal = 0,
    .value_symbol = NULL,
    .count = 0,
    .values = NULL
};

lisp_value_t* lisp_value_new() {
    lisp_value_t* lisp_value = malloc(sizeof(lisp_value_t));
    if (lisp_value == NULL) {
        return NULL;
    }
    lisp_value->error = NO_ERROR;
    lisp_value->value_type = VAL_NUMBER;
    lisp_value->value_number = 0;
    lisp_value->value_decimal = 0;
    lisp_value->value_symbol = NULL;
    lisp_value->count = 0;
    lisp_value->values = NULL;
    return lisp_value;
}

lisp_value_t* lisp_value_number_new(long value) {
    lisp_value_t* lisp_value = lisp_value_new();
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_NUMBER;
    lisp_value->value_number = value;
    return lisp_value;
}

lisp_value_t* lisp_value_decimal_new(double value) {
    lisp_value_t* lisp_value = lisp_value_new();
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_DECIMAL;
    lisp_value->value_decimal = value;
    return lisp_value;
}

lisp_value_t* lisp_value_symbol_new(char* value) {
    lisp_value_t* lisp_value = lisp_value_new();
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_SYMBOL;
    lisp_value->value_symbol = malloc(strlen(value) + 1);
    strcpy(lisp_value->value_symbol, value);
    return lisp_value;
}

lisp_value_t* lisp_value_sexpr_new() {
    lisp_value_t* lisp_value = lisp_value_new();
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_SEXPR;
    return lisp_value;
}

lisp_value_t * lisp_value_root_new() {
    lisp_value_t* lisp_value = lisp_value_new();
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_ROOT;
    return lisp_value;
}

void lisp_value_delete(lisp_value_t* lisp_value) {
    if (lisp_value == &null_lisp_value) {
        return;
    }
    if (lisp_value->value_type == VAL_SEXPR || lisp_value->value_type == VAL_ROOT) {
        for (int i = 0; i < lisp_value->count; i++) {
            lisp_value_delete(lisp_value->values[i]);
        }
        free(lisp_value->values);
    } else if (lisp_value->value_type == VAL_SYMBOL) {
        free(lisp_value->value_symbol);
    }
    free(lisp_value);
}

bool append_lisp_value(lisp_value_t* sexpr, lisp_value_t* value) {
    if (sexpr->value_type == VAL_SEXPR || sexpr->value_type == VAL_ROOT) {
        if (sexpr->values == NULL) {
            sexpr->values = malloc(sizeof(lisp_value_t) * 10);
        } else if (sexpr->count % 10 == 0) {
            lisp_value_t** new_values = realloc(sexpr->values, sizeof(lisp_value_t) * (sexpr->count + 10));
            if (new_values == NULL) {
                return false;
            }
            sexpr->values = new_values;
        }
        sexpr->values[sexpr->count] = value;
        sexpr->count++;
        return true;
    }
    return false;
}

lisp_value_t* lisp_value_error_new(lisp_error_type_t error) {
    lisp_value_t* lisp_error = lisp_value_new();
    if (lisp_error == NULL) {
        return &null_lisp_value;
    }
    lisp_error->error = error;
    return lisp_error;
}

lisp_value_t* get_null_lisp_value() {
    return &null_lisp_value;
}

lisp_value_t * lisp_value_set_child(lisp_value_t *value, int index, lisp_value_t *child) {
    value->values[index] = child;
}

lisp_value_t* lisp_value_pop_child(lisp_value_t *value, int index) {
    /* Find the item at "iindex" */
    lisp_value_t* popped = value->values[index];

    /* Shift memory after the item at "index" over the top */
    memmove(
        &value->values[index],
        &value->values[index + 1],
      sizeof(lisp_value_t*) * (value->count - index -1));

    /* Decrease the count of items in the list */
    value->count--;

    /* Reallocate the memory used, so the values points to a contiguous blocks of `count` lisp_value_t* items */

    /* reallocate with size that is the first value larger or equal to count which is divisible by 10, for example if
     * count is 16, the next larger value divisible by 10 is 20
     *  This is in order to not break the implementation for appending a child to sexpr lisp_value_t*
     * */
    lisp_value_t** new_values = realloc(value->values, sizeof(lisp_value_t*) * (value->count + (10 - value->count % 10)));
    if (new_values == NULL) {
        return &null_lisp_value;
    }
    value->values = new_values;
    return popped;
}

bool is_lisp_value_error(lisp_value_t* lisp_value) {
    return !lisp_value->error == NO_ERROR;
}

bool is_lisp_value_null(lisp_value_t *value) {
    return value == &null_lisp_value;
}

char *get_lisp_value_error_message(lisp_value_t* lisp_value) {
    switch (lisp_value->error) {
        case ERR_INVALID_OPERATOR:
            return ERR_INVALID_OPERATOR_MESSAGE;
        case ERR_DIV_ZERO:
            return ERR_DIV_ZERO_MESSAGE;
        case ERR_BAD_NUMERIC_VALUE:
            return ERR_BAD_NUMERIC_VALUE_MESSAGE;
        case ERR_INCOMPATIBLE_TYPES:
            return ERR_INCOMPATIBLE_TYPES_MESSAGE;
        case ERR_BAD_SEXPR:
            return ERR_BAD_SEXPR_MESSAGE;
        default:
            return UNKNOWN_ERROR_MESSAGE;
    }
}

void print_lisp_expr(lisp_value_t* lisp_value, char open, char close) {
    if (lisp_value->value_type == VAL_SEXPR || lisp_value->value_type == VAL_ROOT) {
        putchar(open);
        for (int i = 0; i < lisp_value->count; i++) {
            print_lisp_value(lisp_value->values[i]);

            if (i + 1 < lisp_value->count) {
                putchar(' ');
            }
        }
        putchar(close);
    }
}

void print_lisp_value(lisp_value_t* lisp_value) {
    if (lisp_value == &null_lisp_value) {
        return;
    }

    if (is_lisp_value_error(lisp_value)) {
        printf("error: %s", get_lisp_value_error_message(lisp_value));
    } else {
        if (lisp_value->value_type == VAL_NUMBER) {
            printf("%ld", lisp_value->value_number);
        } else if (lisp_value->value_type == VAL_DECIMAL) {
            printf("%f", lisp_value->value_decimal);
        } else if (lisp_value->value_type == VAL_SYMBOL) {
            printf("symbol: %s", lisp_value->value_symbol);
        } else if (lisp_value->value_type == VAL_SEXPR || lisp_value->value_type == VAL_ROOT) {
            print_lisp_expr(lisp_value, '(', ')');
        }
    }
}

void print_lisp_eval_result(lisp_eval_result_t *lisp_eval_result) {
    if (lisp_eval_result->error != NULL) {
        printf("error: %s", lisp_eval_result->error);
    } else {
        print_lisp_value(lisp_eval_result->value);
    }
}

//// evaluate non-destructive implementation

lisp_value_t* add_lisp_values(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1->value_number + value2->value_number);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1->value_decimal + value2->value_decimal);
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1->value_number + value2->value_decimal);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1->value_decimal + (double) value2->value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* subtract_lisp_values(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1->value_number - value2->value_number);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1->value_decimal - value2->value_decimal);
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1->value_number - value2->value_decimal);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1->value_decimal - (double) value2->value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* multiply_lisp_values(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1->value_number * value2->value_number);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1->value_decimal * value2->value_decimal);
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1->value_number * value2->value_decimal);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1->value_decimal * (double) value2->value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* divide_lisp_values(lisp_value_t* value1, lisp_value_t* value2) {
    if (value2->value_type == VAL_NUMBER && value2->value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }
    if (value2->value_type == VAL_DECIMAL && value2->value_decimal== 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }

    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        return lisp_value_number_new(value1->value_number / value2->value_number);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value1->value_decimal / value2->value_decimal);
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new((double) value1->value_number / value2->value_decimal);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(value1->value_decimal / (double) value2->value_number);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* division_remainder_lisp_value(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type != VAL_NUMBER || value2->value_type != VAL_NUMBER) {
        return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
    }
    if (value2->value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO);
    }

    return lisp_value_number_new(value1->value_number % value2->value_number);
}

lisp_value_t* pow_lisp_value(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        return lisp_value_number_new((long) pow((double) value1->value_number, (double) value2->value_number));
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(pow(value1->value_decimal, value2->value_decimal));
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(pow((double) value1->value_number, value2->value_decimal));
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        return lisp_value_decimal_new(pow( value1->value_decimal, (double) value2->value_number));
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* min_lisp_value(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        long min = value1->value_number < value2->value_number ? value1->value_number : value2->value_number;
        return lisp_value_number_new(min);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        double min = value1->value_decimal < value2->value_decimal ? value1->value_decimal : value2->value_decimal;
        return lisp_value_decimal_new(min);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* max_lisp_value(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        long max = value1->value_number > value2->value_number ? value1->value_number : value2->value_number;
        return lisp_value_number_new(max);
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        double max = value1->value_decimal > value2->value_decimal ? value1->value_decimal : value2->value_decimal;
        return lisp_value_decimal_new(max);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* negate_lisp_value(lisp_value_t* value) {
    if (value->value_type == VAL_NUMBER) {
        return lisp_value_number_new(-value->value_number);
    }
    if (value->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(-value->value_decimal);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* execute_binary_operation(char* operation, lisp_value_t* first_operand, lisp_value_t* second_operand) {
    if (is_lisp_value_error(first_operand)) {
        return first_operand;
    }
    if (is_lisp_value_error(second_operand)) {
        return second_operand;
    }

    if (strcmp(operation, "+") == 0) {
        return add_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, "-") == 0) {
        return subtract_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, "*") == 0) {
        return multiply_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, "/") == 0) {
        return divide_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, "%") == 0) {
        return division_remainder_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation, "^") == 0) {
        return pow_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation, "min") == 0) {
        lisp_value_t* min = min_lisp_value(first_operand, second_operand);
        return min;
    }
    if (strcmp(operation, "max") == 0) {
        lisp_value_t* max = max_lisp_value(first_operand, second_operand);
        return max;
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR);
}

lisp_value_t* execute_unary_operation(char* operation, lisp_value_t* operand) {
    if (is_lisp_value_error(operand)) {
        return operand;
    }
    if (strcmp(operation, "-") == 0) {
        return negate_lisp_value(operand);
    }
    if (operand->value_type == VAL_NUMBER) {
        return lisp_value_number_new(operand->value_number);
    }
    if (operand->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(operand->value_decimal);
    }
    if (operand->value_type == VAL_SYMBOL) {
        return lisp_value_symbol_new(operand->value_symbol);
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR);
}

/**
 *
 * @param value to be evaluated, can be null_lisp_value, assume is never NULL
 * @return evaluated lisp_value, can be error, can never be null_lisp_value
 */
lisp_value_t* evaluate_lisp_value(lisp_value_t* value) {
    if (value->value_type == VAL_NUMBER) {
        return lisp_value_number_new(value->value_number);
    }
    if (value->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(value->value_decimal);
    }
    if (value->value_type == VAL_SYMBOL) {
        return lisp_value_symbol_new(value->value_symbol);
    }
    if (value->value_type == VAL_SEXPR || value->value_type == VAL_ROOT) {
        if (value->count < 1) {
            return lisp_value_sexpr_new();
        }

        if (value->count == 1) {
            return evaluate_lisp_value(value->values[0]);
        }

        lisp_value_t* operation = value->values[0];
        if (operation->value_type != VAL_SYMBOL) {
            return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
        }

        lisp_value_t* first_operand = evaluate_lisp_value(value->values[1]);
        if (is_lisp_value_error(first_operand)) {
            return first_operand;
        }

        if (value->count == 2) {
            lisp_value_t* result = execute_unary_operation(operation->value_symbol, first_operand);
            lisp_value_delete(first_operand);
            return result;
        }

        for (int i = 2; i < value->count; i++) {
            lisp_value_t* second_operand = evaluate_lisp_value(value->values[i]);
            if (is_lisp_value_error(second_operand)) {
                return second_operand;
            }

            lisp_value_t* next_value = execute_binary_operation(operation->value_symbol, first_operand, second_operand);
            if (is_lisp_value_error(next_value)) {
                lisp_value_delete(second_operand);
                return next_value;
            }

            lisp_value_delete(second_operand);
            lisp_value_delete(first_operand);
            first_operand = next_value;
        }

        return first_operand;
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_eval_result_t* lisp_eval_result_from_lisp_value(lisp_value_t* value) {
    lisp_eval_result_t* result = malloc(sizeof(lisp_eval_result_t));
    result->value = value;
    result->error = NULL;
    return result;
}

lisp_eval_result_t* lisp_eval_result_error_new(char* error_message) {
    lisp_eval_result_t *result = malloc(sizeof(lisp_eval_result_t));
    result->error = malloc( sizeof(char) * (strlen(error_message) + 1));
    strcpy(result->error, error_message);
    result->value = &null_lisp_value;
    return result;
}

lisp_eval_result_t* lisp_eval_result_new(lisp_value_t* value) {
    if (value == &null_lisp_value) {
        return lisp_eval_result_error_new("null lisp_value");
    }
    if (is_lisp_value_error(value)) {
        return lisp_eval_result_error_new(get_lisp_value_error_message(value));
    }
    return lisp_eval_result_from_lisp_value(value);
}

void lisp_eval_result_delete(lisp_eval_result_t* lisp_eval_result) {
    lisp_value_delete(lisp_eval_result->value);
    if (lisp_eval_result->error != NULL) {
        free(lisp_eval_result->error);
    }
    free(lisp_eval_result);
}

/* Evaluates lisp_value_t* in a non-destructive way,
 * meaning, calling this function with the same parameter multiple times should work and evaluate the same as the first call */
lisp_eval_result_t* evaluate_root_lisp_value(lisp_value_t* value) {
    if (value == &null_lisp_value || value->value_type != VAL_ROOT) {
        return lisp_eval_result_error_new("invalid root lisp value");
    }

    /* this code is valid if we treat root as SEXPR */
    lisp_value_t* evaluated = evaluate_lisp_value(value);
    return lisp_eval_result_from_lisp_value(evaluated);

    /* this code is valid if we don't treat root as SEXPR */
    // lisp_value_t* evaluated = &null_lisp_value;
    // for (int i = 0; i < value->count; i++) {
    //     evaluated = evaluate_lisp_value(value->values[i]);
    //     if (is_lisp_value_error(evaluated) || is_lisp_value_null(evaluated)) {
    //         lisp_eval_result_t* eval_result = lisp_eval_result_new(evaluated);
    //         lisp_value_delete(evaluated);
    //         return eval_result;
    //     }
    //     if (i + 1 != value->count) {
    //         lisp_value_delete(evaluated);
    //     }
    // }
    // return lisp_eval_result_new(evaluated);
}

//// end evaluate non-destructive implementation

//// evaluate destructive implementation

lisp_value_t * negate_lisp_value_destructive(lisp_value_t * value) {
    if (value->value_type == VAL_NUMBER) {
        value->value_number = -value->value_number;
        return value;
    }
    if (value->value_type == VAL_DECIMAL) {
        value->value_decimal = -value->value_decimal;
        return value;
    }

    lisp_value_t* value_error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
    lisp_value_delete(value);
    return value_error;
}

lisp_value_t* execute_unary_operation_destructive(char* operation, lisp_value_t* operand) {
    if (operand == &null_lisp_value) {
        return &null_lisp_value;
    }

    if (strcmp(operation, "-") == 0) {
        return negate_lisp_value_destructive(operand);
    }
    return operand;
}

lisp_value_t* add_lisp_values_destructive(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_NUMBER) {
        lisp_value_t* result = lisp_value_number_new(value1->value_number + value2->value_number);
        lisp_value_delete(value1);
        lisp_value_delete(value2);
        return result;
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_DECIMAL) {
        lisp_value_t* result = lisp_value_decimal_new(value1->value_decimal + value2->value_decimal);
        lisp_value_delete(value1);
        lisp_value_delete(value2);
        return result;
    }
    if (value1->value_type == VAL_NUMBER && value2->value_type == VAL_DECIMAL) {
        lisp_value_t* result = lisp_value_decimal_new((double) value1->value_number + value2->value_decimal);
        lisp_value_delete(value1);
        lisp_value_delete(value2);
        return result;
    }
    if (value1->value_type == VAL_DECIMAL && value2->value_type == VAL_NUMBER) {
        lisp_value_t* result = lisp_value_decimal_new(value1->value_decimal + (double) value2->value_number);
        lisp_value_delete(value1);
        lisp_value_delete(value2);
        return result;
    }

    lisp_value_delete(value1);
    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES);
}

lisp_value_t* execute_binary_operation_destructive(char* operation, lisp_value_t* first_operand, lisp_value_t* second_operand) {
    if (first_operand == &null_lisp_value || second_operand == &null_lisp_value) {
        lisp_value_delete(first_operand);
        lisp_value_delete(second_operand);
        return &null_lisp_value;
    }

    if (strcmp(operation, "+") == 0) {
        return add_lisp_values_destructive(first_operand, second_operand);
    }

    lisp_value_delete(first_operand);
    lisp_value_delete(second_operand);
    return lisp_value_error_new(ERR_INVALID_OPERATOR);
}

/* Assumes value is sexpr of one operator and at least one operand */
lisp_value_t* builtin_operation(lisp_value_t* value) {
    lisp_value_t* operation = lisp_value_pop_child(value, 0);
    if (operation == &null_lisp_value) {
        lisp_value_delete(value);
        return &null_lisp_value;
    }
    if (operation->value_type != VAL_SYMBOL) {
        lisp_value_delete(operation);
        lisp_value_delete(value);
        return lisp_value_error_new(ERR_INVALID_OPERATOR);
    }

    lisp_value_t* first_operand = execute_unary_operation_destructive(operation->value_symbol, lisp_value_pop_child(value, 0));
    while (value->count > 0) {
        if (first_operand == &null_lisp_value) {
            lisp_value_delete(operation);
            lisp_value_delete(value);
            return &null_lisp_value;
        }
        if (is_lisp_value_error(first_operand)) {
            return first_operand;
        }
        lisp_value_t* next_value = execute_binary_operation_destructive(operation->value_symbol, first_operand, lisp_value_pop_child(value, 0));
        first_operand = next_value;
    }

    lisp_value_delete(operation);
    lisp_value_delete(value);
    return first_operand;
}

lisp_value_t* evaluate_lisp_value_destructive(lisp_value_t* value) {
    if (value == &null_lisp_value) {
        return value;
    }
    if (value->value_type == VAL_NUMBER) {
        return value;
    }
    if (value->value_type == VAL_DECIMAL) {
        return value;
    }
    if (value->value_type == VAL_SYMBOL) {
        return value;
    }

    if (value->value_type == VAL_SEXPR || value->value_type == VAL_ROOT) {
        for (int i = 0; i < value->count; i++) {
            lisp_value_set_child(value, i, evaluate_lisp_value_destructive(value->values[i]));
            if (is_lisp_value_error(value->values[i])) {
                lisp_value_t* error_value = lisp_value_error_new(value->values[i]->error);
                lisp_value_delete(value);
                return error_value;
            }
            if (value->values[i] == &null_lisp_value) {
                lisp_value_delete(value);
                return &null_lisp_value;
            }
        }

        if (value->count < 1) {
            return value;
        }

        if (value->count == 1) {
            lisp_value_t* evaluated_child = lisp_value_pop_child(value, 0);
            lisp_value_delete(value);
            return evaluated_child;
        }

        return builtin_operation(value);
    }
}

/* Evaluates lisp_value_t* in a destructive way,
 * meaning, calling this function with the same parameter multiple times should work and evaluate the same as the first call
 *
 * Additional implementation notes:
 * The functions that return a new lisp_value_t* will have the responsibility of deleting the input lisp_value_t*
 */
lisp_eval_result_t* evaluate_root_lisp_value_destructive(lisp_value_t* value) {
    if (value == &null_lisp_value || value->value_type != VAL_ROOT) {
        lisp_value_delete(value);
        return lisp_eval_result_error_new("invalid root lisp value");
    }

    /* this code is valid if we treat root as SEXPR */
    lisp_value_t* evaluated = evaluate_lisp_value_destructive(value);
    return lisp_eval_result_new(evaluated);
}

//// end evaluate destructive implementation