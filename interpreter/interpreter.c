#include "interpreter.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static int MAX_ERROR_MESSAGE_BUFF_SIZE = 512;

static char* BUILTIN_PLUS = "+";
static char* BUILTIN_MINUS = "-";
static char* BUILTIN_MULTIPLY = "*";
static char* BUILTIN_DIVIDE = "/";
static char* BUILTIN_MOD = "%";
static char* BUILTIN_POW = "^";
static char* BUILTIN_MIN = "min";
static char* BUILTIN_MAX = "max";
static char* BUILTIN_LIST = "list";
static char* BUILTIN_HEAD = "head";
static char* BUILTIN_TAIL = "tail";
static char* BUILTIN_JOIN = "join";
static char* BUILTIN_EVAL = "eval";
static char* BUILTIN_CONS = "cons";
static char* BUILTIN_LEN = "len";
static char* BUILTIN_INIT = "init";
static char* BUILTIN_DEF = "def";

static lisp_value_t null_lisp_value = {
    .value_type = 0,
    .error_message = NULL,
    .value_number = 0,
    .value_decimal = 0,
    .value_symbol = NULL,
    .count = 0,
    .values = NULL
};

static lisp_environment_t null_lisp_environment = {
    .symbols = NULL,
    .values = NULL
};

lisp_value_t* lisp_value_new(lisp_value_type_t value_type) {
    lisp_value_t* lisp_value = malloc(sizeof(lisp_value_t));
    if (lisp_value == NULL) {
        return NULL;
    }
    lisp_value->value_type = value_type;
    lisp_value->error_message = NULL;
    lisp_value->value_number = 0;
    lisp_value->value_decimal = 0;
    lisp_value->value_symbol = NULL;
    lisp_value->count = 0;
    lisp_value->values = NULL;
    return lisp_value;
}

char* get_value_type_string(lisp_value_type_t value_type) {
    switch (value_type) {
        case VAL_ERR:
            return "Error";
        case VAL_NUMBER:
            return "Number";
        case VAL_DECIMAL:
            return "Decimal";
        case VAL_SYMBOL:
            return "Symbol";
        case VAL_SEXPR:
            return "S-expression";
        case VAL_ROOT:
            return "Root-expression";
        case VAL_QEXPR:
            return "Q-expression";
        case VAL_BUILTIN_FUN:
            return "Built-in";
        default:
            return "Unknown type";
    }
}

lisp_value_t* lisp_value_number_new(long value) {
    lisp_value_t* lisp_value = lisp_value_new(VAL_NUMBER);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_number = value;
    return lisp_value;
}

lisp_value_t* lisp_value_decimal_new(double value) {
    lisp_value_t* lisp_value = lisp_value_new(VAL_DECIMAL);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_decimal = value;
    return lisp_value;
}

lisp_value_t* lisp_value_symbol_new(char* value) {
    lisp_value_t* lisp_value = lisp_value_new(VAL_SYMBOL);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_symbol = malloc(strlen(value) + 1);
    if (lisp_value->value_symbol == NULL) {
        lisp_value_delete(lisp_value);
        return &null_lisp_value;
    }
    strcpy(lisp_value->value_symbol, value);
    return lisp_value;
}

lisp_value_t* lisp_value_sexpr_new() {
    lisp_value_t* lisp_value = lisp_value_new(VAL_SEXPR);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    return lisp_value;
}

lisp_value_t * lisp_value_root_new() {
    lisp_value_t* lisp_value = lisp_value_new(VAL_ROOT);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    return lisp_value;
}

lisp_value_t * lisp_value_qexpr_new() {
    lisp_value_t* lisp_value = lisp_value_new(VAL_QEXPR);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    return lisp_value;
}

void lisp_value_delete(lisp_value_t* lisp_value) {
    if (lisp_value == &null_lisp_value) {
        return;
    }

    if (lisp_value->value_type == VAL_SEXPR || lisp_value->value_type == VAL_ROOT || lisp_value->value_type == VAL_QEXPR) {
        for (int i = 0; i < lisp_value->count; i++) {
            lisp_value_delete(lisp_value->values[i]);
        }
        free(lisp_value->values);
    } else if (lisp_value->value_type == VAL_SYMBOL || lisp_value->value_type == VAL_BUILTIN_FUN) {
        free(lisp_value->value_symbol);
    } else if (lisp_value->value_type == VAL_ERR) {
        free(lisp_value->error_message);
    }
    free(lisp_value);
}

lisp_value_t * lisp_value_builtin_fun_new(char *symbol) {
    lisp_value_t* lisp_value = lisp_value_symbol_new(symbol);
    if (lisp_value == &null_lisp_value) {
        return &null_lisp_value;
    }
    lisp_value->value_type = VAL_BUILTIN_FUN;
    return lisp_value;
}

lisp_value_t* lisp_value_error_new(char* error_message_template, ...) {
    lisp_value_t* lisp_error = lisp_value_new(VAL_ERR);
    if (lisp_error == NULL) {
        return &null_lisp_value;
    }

    va_list va;
    va_start(va, error_message_template);

    bool ok = false;
    lisp_error->error_message = malloc(sizeof(char) * MAX_ERROR_MESSAGE_BUFF_SIZE);
    if (lisp_error->error_message != NULL) {
        vsnprintf(lisp_error->error_message, MAX_ERROR_MESSAGE_BUFF_SIZE - 1, error_message_template, va);
        char* error_message = realloc(lisp_error->error_message, strlen(lisp_error->error_message) + 1);
        if (error_message != NULL) {
            lisp_error->error_message = error_message;
            ok = true;
        }
    }

    va_end(va);
    if (!ok) {
        lisp_value_delete(lisp_error);
        lisp_error = &null_lisp_value;
    }

    return lisp_error;
}

lisp_value_t * lisp_value_copy(lisp_value_t *value) {
    lisp_value_t* copy = lisp_value_new(value->value_type);
    if (copy == NULL) {
        return &null_lisp_value;
    }

    bool ok = true;
    switch (value->value_type) {
        case VAL_ERR:
            copy->error_message = malloc(strlen(value->error_message) + 1);
            if (copy->error_message == NULL) {
                ok = false;
                break;
            }
            strcpy(copy->error_message, value->error_message);
            break;
        case VAL_NUMBER:
            copy->value_number = value->value_number;
            break;
        case VAL_DECIMAL:
            copy->value_decimal = value->value_decimal;
            break;
        case VAL_SYMBOL:
            case VAL_BUILTIN_FUN:
            copy->value_symbol = malloc(strlen(value->value_symbol) + 1);
            if (copy->value_symbol == NULL) {
                ok = false;
                break;
            }
            strcpy(copy->value_symbol, value->value_symbol);
            break;
        case VAL_SEXPR:
        case VAL_ROOT:
        case VAL_QEXPR:
            copy->count = value->count;
            copy->values = malloc(sizeof(lisp_value_t*) * value->count);
            if (copy->values == NULL) {
                ok = false;
                break;
            }
            for (int i = 0; i < value->count; i++) {
                copy->values[i] = lisp_value_copy(value->values[i]);
            }
            break;
    }

    if (!ok) {
        lisp_value_delete(copy);
        return &null_lisp_value;
    }

    return copy;
}

lisp_value_t* get_null_lisp_value() {
    return &null_lisp_value;
}

bool should_contain_children(lisp_value_t* value) {
    return value != &null_lisp_value
    && (value->value_type == VAL_ROOT || value->value_type == VAL_SEXPR || value->value_type == VAL_QEXPR);
}

bool append_lisp_value(lisp_value_t* value, lisp_value_t* child_to_append) {
    if (should_contain_children(value)) {
        if (value->values == NULL) {
            value->values = malloc(sizeof(lisp_value_t) * 10);
        } else if (value->count > 0 && value->count % 10 == 0) {
            lisp_value_t** new_values = realloc(value->values, sizeof(lisp_value_t*) * (value->count + 10));
            if (new_values == NULL) {
                return false;
            }
            value->values = new_values;
        }
        if (value->values == NULL) {
            return false;
        }
        value->values[value->count] = child_to_append;
        value->count++;
        return true;
    }
    return false;
}

void lisp_value_set_child(lisp_value_t *value, int index, lisp_value_t *child) {
    if (should_contain_children(value)) {
        value->values[index] = child;
    }
}

lisp_value_t* lisp_value_pop_child(lisp_value_t *value, int index) {
    if (!should_contain_children(value)) {
        return &null_lisp_value;
    }

    /* Find the item at "index" */
    lisp_value_t* popped = value->values[index];

    /* Shift memory after the item at "index" over the top */
    memmove(
        &value->values[index],
        &value->values[index + 1],
      sizeof(lisp_value_t*) * (value->count - index -1));

    /* Decrease the count of items in the list */
    value->count--;
    if (value->count < 0) {
        value->count = 0;
    }

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
    return lisp_value->value_type == VAL_ERR;
}

bool is_lisp_value_null(lisp_value_t *value) {
    return value == &null_lisp_value;
}

void print_lisp_value_with_children(lisp_value_t* lisp_value, char open, char close) {
    if (should_contain_children(lisp_value)) {
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

    switch (lisp_value->value_type) {
        case VAL_ERR:
            printf("error: %s", lisp_value->error_message);
        break;
        case VAL_NUMBER:
            printf("%ld", lisp_value->value_number);
        break;
        case VAL_DECIMAL:
            printf("%f", lisp_value->value_decimal);
        break;
        case VAL_SYMBOL:
            printf("symbol: %s", lisp_value->value_symbol);
        break;
        case VAL_SEXPR:
        case VAL_ROOT:
            print_lisp_value_with_children(lisp_value, '(', ')');
        break;
        case VAL_QEXPR:
            print_lisp_value_with_children(lisp_value, '{', '}');
        break;
        case VAL_BUILTIN_FUN:
            printf("builtin: %s", lisp_value->value_symbol);
        break;
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
}

lisp_value_t* divide_lisp_values(lisp_value_t* value1, lisp_value_t* value2) {
    if (value2->value_type == VAL_NUMBER && value2->value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
    }
    if (value2->value_type == VAL_DECIMAL && value2->value_decimal== 0) {
        return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
}

lisp_value_t* division_remainder_lisp_value(lisp_value_t* value1, lisp_value_t* value2) {
    if (value1->value_type != VAL_NUMBER || value2->value_type != VAL_NUMBER) {
        return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
    }
    if (value2->value_number == 0) {
        return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
}

lisp_value_t* negate_lisp_value(lisp_value_t* value) {
    if (value->value_type == VAL_NUMBER) {
        return lisp_value_number_new(-value->value_number);
    }
    if (value->value_type == VAL_DECIMAL) {
        return lisp_value_decimal_new(-value->value_decimal);
    }

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
}

lisp_value_t* execute_binary_operation(char* operation, lisp_value_t* first_operand, lisp_value_t* second_operand) {
    if (is_lisp_value_error(first_operand)) {
        return first_operand;
    }
    if (is_lisp_value_error(second_operand)) {
        return second_operand;
    }

    if (strcmp(operation, BUILTIN_PLUS) == 0) {
        return add_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_MINUS) == 0) {
        return subtract_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_MULTIPLY) == 0) {
        return multiply_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_DIVIDE) == 0) {
        return divide_lisp_values(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_MOD) == 0) {
        return division_remainder_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_POW) == 0) {
        return pow_lisp_value(first_operand, second_operand);
    }
    if (strcmp(operation, BUILTIN_MIN) == 0) {
        lisp_value_t* min = min_lisp_value(first_operand, second_operand);
        return min;
    }
    if (strcmp(operation, BUILTIN_MAX) == 0) {
        lisp_value_t* max = max_lisp_value(first_operand, second_operand);
        return max;
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
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
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
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
            return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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

    return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
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
        lisp_eval_result_t* result = lisp_eval_result_error_new(value->error_message);
        lisp_value_delete(value);
        return result;
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

lisp_value_t* numeric_op_number(char* operation, long value1, long value2) {
    if (operation[0] == '+') {
        return lisp_value_number_new(value1 + value2);
    }
    if (operation[0] == '-') {
        return lisp_value_number_new(value1 - value2);
    }
    if (operation[0] == '*') {
        return lisp_value_number_new(value1 * value2);
    }
    if (operation[0] == '/') {
        if (value2 == 0) {
            return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
        }
        return lisp_value_number_new(value1 / value2);
    }
    if (operation[0] == '%') {
        if (value2 == 0) {
            return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
        }
        return lisp_value_number_new(value1 % value2);
    }
    if (operation[0] == '^') {
        return lisp_value_number_new((long) pow((double) value1, (double) value2));
    }
    if (strcmp(operation, BUILTIN_MIN) == 0) {
        return lisp_value_number_new(value1 < value2 ? value1 : value2);
    }
    if (strcmp(operation, BUILTIN_MAX) == 0) {
        return lisp_value_number_new(value1 > value2 ? value1 : value2);
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
}

lisp_value_t* numeric_op_decimal(char* operation, double value1, double value2) {
    if (operation[0] == '+') {
        return lisp_value_decimal_new(value1 + value2);
    }
    if (operation[0] == '-') {
        return lisp_value_decimal_new(value1 - value2);
    }
    if (operation[0] == '*') {
        return lisp_value_decimal_new(value1 * value2);
    }
    if (operation[0] == '/') {
        if (value2 == 0) {
            return lisp_value_error_new(ERR_DIV_ZERO_MESSAGE);
        }
        return lisp_value_decimal_new(value1 / value2);
    }
    if (operation[0] == '%') {
        return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
    }
    if (operation[0] == '^') {
        return lisp_value_decimal_new(pow(value1, value2));
    }
    if (strcmp(operation, BUILTIN_MIN) == 0) {
        return lisp_value_decimal_new(value1 < value2 ? value1 : value2);
    }
    if (strcmp(operation, BUILTIN_MAX) == 0) {
        return lisp_value_decimal_new(value1 > value2 ? value1 : value2);
    }
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
}

#define ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, builtin_fun) \
    do {\
        if (arguments->count != 1) {\
            lisp_value_t* error = lisp_value_error_new(ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE, builtin_fun, 1, arguments->count);\
            lisp_value_delete(arguments);\
            return error;\
        }\
\
        if (arguments->values[0]->value_type != VAL_QEXPR) {\
            lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, builtin_fun, get_value_type_string(VAL_QEXPR), get_value_type_string(arguments->values[0]->value_type));\
            lisp_value_delete(arguments);\
            return error;\
        }\
    } while (0);

lisp_value_t* builtin_head(lisp_value_t* arguments) {
    ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, BUILTIN_HEAD);

    lisp_value_t* qexpr = lisp_value_pop_child(arguments, 0);

    lisp_value_t* head = lisp_value_qexpr_new();
    if (qexpr->count > 0) {
        bool ok = append_lisp_value(head, lisp_value_pop_child(qexpr, 0));
        if (!ok) {
            lisp_value_delete(head);
            head = &null_lisp_value;
        }
    }

    lisp_value_delete(arguments);
    lisp_value_delete(qexpr);
    return head;
}

lisp_value_t* builtin_tail(lisp_value_t* arguments) {
    ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, BUILTIN_TAIL);

    lisp_value_t* qexpr = lisp_value_pop_child(arguments, 0);

    if (qexpr->count < 1) {
        lisp_value_delete(arguments);
        return qexpr;
    }

    lisp_value_t* head = lisp_value_pop_child(qexpr, 0);
    lisp_value_delete(head);
    lisp_value_delete(arguments);
    return qexpr;
}

lisp_value_t* builtin_join(lisp_value_t* arguments) {
    for (int i = 0; i < arguments->count; i++) {
        if (arguments->values[i]->value_type != VAL_QEXPR) {
            lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, i + 1, BUILTIN_JOIN, get_value_type_string(VAL_QEXPR), get_value_type_string(arguments->values[i]->value_type));
            lisp_value_delete(arguments);
            return error;
        }
    }

    lisp_value_t* result = lisp_value_pop_child(arguments, 0);
    while (arguments->count > 0) {
        lisp_value_t* current_qexpr = lisp_value_pop_child(arguments, 0);
        while (current_qexpr->count > 0) {
            bool ok = append_lisp_value(result, lisp_value_pop_child(current_qexpr, 0));
            if (!ok) {
                lisp_value_delete(result);
                lisp_value_delete(arguments);
                return &null_lisp_value;
            }
        }
        lisp_value_delete(current_qexpr);
    }

    lisp_value_delete(arguments);
    return result;
}

lisp_value_t* builtin_eval(lisp_environment_t* env, lisp_value_t* arguments) {
    ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, BUILTIN_EVAL);
    lisp_value_t* qexpr = lisp_value_pop_child(arguments, 0);
    qexpr->value_type = VAL_SEXPR;
    lisp_value_t* result = evaluate_lisp_value_destructive(env, qexpr);
    lisp_value_delete(arguments);
    return result;
}

lisp_value_t* builtin_cons(lisp_value_t* arguments) {
    if (arguments->count != 2) {
        lisp_value_t* error = lisp_value_error_new(ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE, BUILTIN_CONS, 2, arguments->count);
        lisp_value_delete(arguments);
        return error;
    }

    lisp_value_t* value_to_cons = lisp_value_pop_child(arguments, 0);
    lisp_value_t* existing_qexpr = lisp_value_pop_child(arguments, 0);

    if (existing_qexpr->value_type != VAL_QEXPR) {
        lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 2, BUILTIN_CONS, get_value_type_string(VAL_QEXPR), get_value_type_string(existing_qexpr->value_type));
        lisp_value_delete(existing_qexpr);
        lisp_value_delete(value_to_cons);
        lisp_value_delete(arguments);
        return error;
    }

    lisp_value_t* result_qexpr = lisp_value_qexpr_new();
    bool ok = append_lisp_value(result_qexpr, value_to_cons);
    if (!ok) {
        lisp_value_delete(result_qexpr);
        lisp_value_delete(existing_qexpr);
        lisp_value_delete(value_to_cons);
        lisp_value_delete(arguments);
        return &null_lisp_value;
    }

    while (existing_qexpr->count > 0) {
        ok = append_lisp_value(result_qexpr, lisp_value_pop_child(existing_qexpr, 0));
        if (!ok) {
            lisp_value_delete(result_qexpr);
            lisp_value_delete(existing_qexpr);
            lisp_value_delete(value_to_cons);
            lisp_value_delete(arguments);
            return &null_lisp_value;
        }
    }

    lisp_value_delete(existing_qexpr);
    lisp_value_delete(arguments);
    return result_qexpr;
}

lisp_value_t* builtin_len(lisp_value_t* arguments) {
    ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, BUILTIN_LEN);
    lisp_value_t* qexpr = lisp_value_pop_child(arguments, 0);
    lisp_value_t* result = lisp_value_number_new(qexpr->count);
    lisp_value_delete(arguments);
    return result;
}

lisp_value_t* builtin_init(lisp_value_t* arguments) {
    ASSERT_ARGUMENTS_REPRESENT_ONE_QEXPR(arguments, BUILTIN_INIT);
    lisp_value_t* qexpr = lisp_value_pop_child(arguments, 0);
    if (qexpr->count < 1) {
        return qexpr;
    }
    lisp_value_t* last_child = lisp_value_pop_child(qexpr, qexpr->count - 1);
    lisp_value_delete(last_child);
    return qexpr;
}

lisp_value_t* builtin_def(lisp_environment_t* env, lisp_value_t* arguments) {
    lisp_value_t* qexpr_of_symbols = lisp_value_pop_child(arguments, 0);
    if (qexpr_of_symbols->value_type != VAL_QEXPR) {
        lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, BUILTIN_DEF, get_value_type_string(VAL_QEXPR), get_value_type_string(qexpr_of_symbols->value_type));
        lisp_value_delete(qexpr_of_symbols);
        lisp_value_delete(arguments);
        return error;
    }

    if (qexpr_of_symbols->count != arguments->count) {
        lisp_value_t* error = lisp_value_error_new(ERR_BUILTIN_DEF_INVALID_VALUE_COUNT_MESSAGE_TEMPLATE, qexpr_of_symbols->count, arguments->count);
        lisp_value_delete(qexpr_of_symbols);
        lisp_value_delete(arguments);
        return error;
    }

    for (int i = 0; i < qexpr_of_symbols->count; i++) {
        if (qexpr_of_symbols->values[i]->value_type != VAL_SYMBOL) {
            lisp_value_t* error = lisp_value_error_new(ERR_BUILTIN_DEF_INVALID_TYPE_FOR_VARIABLE_NAME_MESSAGE_TEMPLATE, get_value_type_string(VAL_SYMBOL), get_value_type_string(qexpr_of_symbols->values[i]->value_type));
            lisp_value_delete(qexpr_of_symbols);
            lisp_value_delete(arguments);
            return error;
        }
        lisp_value_t* value = lisp_environment_get(env, qexpr_of_symbols->values[i]);
        if (value->value_type == VAL_BUILTIN_FUN) {
            lisp_value_t* error = lisp_value_error_new(ERR_NOT_ALLOWED_TO_REDEFINE_BUILTIN_FUN_MESSAGE_TEMPLATE, value->value_symbol);
            lisp_value_delete(value);
            lisp_value_delete(qexpr_of_symbols);
            lisp_value_delete(arguments);
            return error;
        }
        lisp_value_delete(value);
    }

    for (int i = 0; i < qexpr_of_symbols->count; i++) {
        lisp_environment_set(env, qexpr_of_symbols->values[i], arguments->values[i]);
    }

    lisp_value_delete(qexpr_of_symbols);
    lisp_value_delete(arguments);
    return lisp_value_sexpr_new();
}

lisp_value_t* builtin_operation_for_numeric_arguments(char* operation, lisp_value_t* arguments) {
    if (arguments->count < 1) {
        lisp_value_t* error = lisp_value_error_new(ERR_AT_LEAST_ONE_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE, operation);
        lisp_value_delete(arguments);
        return error;
    }

    long result_number = arguments->values[0]->value_type == VAL_NUMBER
        ? arguments->values[0]->value_number
        : (long) arguments->values[0]->value_decimal;
    double result_decimal = arguments->values[0]->value_type == VAL_NUMBER
        ? (double) arguments->values[0]->value_number
        : arguments->values[0]->value_decimal;
    bool is_prev_decimal = arguments->values[0]->value_type == VAL_DECIMAL ? true : false;
    bool should_return_decimal = arguments->values[0]->value_type == VAL_DECIMAL ? true : false;

    for (int i = 0; i < arguments->count; i++) {
        if (arguments->values[i]->value_type != VAL_NUMBER && arguments->values[i]->value_type != VAL_DECIMAL) {
            lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, i + 1, operation, "Number or Decimal", get_value_type_string(arguments->values[i]->value_type));
            lisp_value_delete(arguments);
            return error;
        }

        // min and max should return non-error if all numeric values are only of one type
        if (strcmp(operation, BUILTIN_MIN) == 0 || strcmp(operation, BUILTIN_MAX) == 0) {
            if ((is_prev_decimal && arguments->values[i]->value_type == VAL_NUMBER)|| (!is_prev_decimal && arguments->values[i]->value_type == VAL_DECIMAL)) {
                lisp_value_delete(arguments);
                return lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE);
            }
        }

        is_prev_decimal = arguments->values[i]->value_type == VAL_DECIMAL ? true : false;
        if (arguments->values[i]->value_type == VAL_DECIMAL) {
            should_return_decimal = true;
        }

        if (i == 0) {
            continue;
        }

        long number = arguments->values[i]->value_type == VAL_NUMBER ? arguments->values[i]->value_number : (long) arguments->values[i]->value_decimal;
        double number_decimal = arguments->values[i]->value_type == VAL_DECIMAL ? arguments->values[i]->value_decimal : (double) arguments->values[i]->value_number;

        lisp_value_t* r = numeric_op_number(operation, result_number, number);
        if (!should_return_decimal && is_lisp_value_error(r)) {
            lisp_value_delete(arguments);
            return r;
        }
        result_number = r->value_number;

        lisp_value_t* r1 = numeric_op_decimal(operation, result_decimal, number_decimal);
        if (should_return_decimal && is_lisp_value_error(r1)) {
            lisp_value_delete(r);
            lisp_value_delete(arguments);
            return r1;
        }

        result_decimal = r1->value_decimal;
        lisp_value_delete(r1);
        lisp_value_delete(r);
    }

    if (operation[0] == '-' && arguments->count == 1) {
        result_number = -result_number;
        result_decimal = -result_decimal;
    }

    lisp_value_delete(arguments);
    if (should_return_decimal) {
        return lisp_value_decimal_new(result_decimal);
    }
    return lisp_value_number_new(result_number);
}

/* Assumes value is sexpr of one operator(builtin fun) and at least one operand and all operands are previously evaluated */
lisp_value_t* builtin_operation(lisp_environment_t* env, lisp_value_t* value) {
    lisp_value_t* operation = lisp_value_pop_child(value, 0);
    if (operation == &null_lisp_value) {
        lisp_value_delete(value);
        return &null_lisp_value;
    }
    if (operation->value_type != VAL_BUILTIN_FUN) {
        lisp_value_delete(operation);
        lisp_value_delete(value);
        return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
    }

    if (strpbrk(operation->value_symbol, "+-*/^%") != NULL
        || strcmp(operation->value_symbol, BUILTIN_MIN) == 0 || strcmp(operation->value_symbol, BUILTIN_MAX) == 0) {
        lisp_value_t* result = builtin_operation_for_numeric_arguments(operation->value_symbol, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_LIST) == 0) {
        value->value_type = VAL_QEXPR;
        lisp_value_delete(operation);
        return value;
    }

    if (strcmp(operation->value_symbol, BUILTIN_HEAD) == 0) {
        lisp_value_t* result = builtin_head(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_TAIL) == 0) {
        lisp_value_t* result = builtin_tail(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_JOIN) == 0) {
        lisp_value_t* result = builtin_join(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_EVAL) == 0) {
        lisp_value_t* result = builtin_eval(env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_CONS) == 0) {
        lisp_value_t* result = builtin_cons(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_LEN) == 0) {
        lisp_value_t* result = builtin_len(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_INIT) == 0) {
        lisp_value_t* result = builtin_init(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_DEF) == 0) {
        lisp_value_t* result = builtin_def(env, value);
        lisp_value_delete(operation);
        return result;
    }

    lisp_value_delete(operation);
    lisp_value_delete(value);
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
}

lisp_value_t* evaluate_lisp_value_destructive(lisp_environment_t *env, lisp_value_t* value) {
    if (value->value_type == VAL_SYMBOL) {
        lisp_value_t* result = lisp_environment_get(env, value);
        lisp_value_delete(value);
        return result;
    }
    if (value->value_type == VAL_SEXPR || value->value_type == VAL_ROOT) {
        for (int i = 0; i < value->count; i++) {
            lisp_value_set_child(value, i, evaluate_lisp_value_destructive(env, value->values[i]));
            if (is_lisp_value_error(value->values[i])) {
                lisp_value_t* error_value = lisp_value_error_new(value->values[i]->error_message);
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

        return builtin_operation(env, value);
    }

    return value;
}

/* Evaluates lisp_value_t* in a destructive way,
 * meaning, calling this function with the same parameter multiple times should work and evaluate the same as the first call
 *
 * Additional implementation notes:
 * The functions that return a new lisp_value_t* will have the responsibility of deleting the input lisp_value_t*
 */
lisp_eval_result_t* evaluate_root_lisp_value_destructive(lisp_environment_t* env, lisp_value_t* value) {
    if (value == &null_lisp_value || value->value_type != VAL_ROOT) {
        lisp_value_delete(value);
        return lisp_eval_result_error_new("invalid root lisp value");
    }

    /* this code is valid if we treat root as SEXPR */
    lisp_value_t* evaluated = evaluate_lisp_value_destructive(env, value);
    return lisp_eval_result_new(evaluated);
}

//// end evaluate destructive implementation

lisp_environment_t * lisp_environment_new() {
    lisp_environment_t* env = malloc(sizeof(lisp_environment_t));
    if (env == NULL) {
        return &null_lisp_environment;
    }

    env->count = 0;
    env->symbols = malloc(sizeof(char*) * 10);
    env->values = malloc(sizeof(lisp_value_t*) * 10);
    if (env->symbols == NULL || env->values == NULL) {
        lisp_environment_delete(env);
        return &null_lisp_environment;
    }

    return env;
}

void lisp_environment_delete(lisp_environment_t *env) {
    if (env == &null_lisp_environment) {
        return;
    }

    if (env->symbols != NULL) {
        for (size_t i = 0; i < env->count; i++) {
            free(env->symbols[i]);
        }
    }

    if (env->values != NULL) {
        for (size_t i = 0; i < env->count; i++) {
            lisp_value_delete(env->values[i]);
        }
    }

    free(env->symbols);
    free(env->values);
    free(env);
}

bool lisp_environment_set(lisp_environment_t* env, lisp_value_t *symbol, lisp_value_t *value) {
    if (env == &null_lisp_environment) {
        return false;
    }
    if (symbol == &null_lisp_value) {
        return false;
    }

    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(symbol->value_symbol, env->symbols[i]) == 0) {
            lisp_value_delete(env->values[i]);
            env->values[i] = lisp_value_copy(value);
            return true;
        }
    }

    bool ok = true;
    char** symbols_new = env->symbols;
    lisp_value_t** values_new = env->values;
    if (env->count > 0 && env->count % 10 == 0) {
        if (ok) {
            symbols_new = realloc(env->symbols, sizeof(char*) * (env->count + 10));
            if (symbols_new == NULL) {
                ok = false;
            }
        }

        if (ok) {
            values_new = realloc(env->values, sizeof(lisp_value_t*) * (env->count + 10));
            if (values_new == NULL) {
                ok = false;
            }
        }
    }

    if (ok) {
        env->symbols = symbols_new;
        env->values = values_new;

        env->symbols[env->count] = malloc(strlen(symbol->value_symbol) + 1);
        if (env->symbols[env->count] != NULL) {
            strcpy(env->symbols[env->count], symbol->value_symbol);
            env->values[env->count] = lisp_value_copy(value);
            env->count++;
            return true;
        }
    }

    return false;
}

lisp_value_t* lisp_environment_get(lisp_environment_t* env, lisp_value_t *symbol) {
    if (env == &null_lisp_environment) {
        return &null_lisp_value;
    }
    if (symbol == &null_lisp_value) {
        return &null_lisp_value;
    }

    lisp_value_t* result = &null_lisp_value;
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(symbol->value_symbol, env->symbols[i]) == 0) {
            result = lisp_value_copy(env->values[i]);
            break;
        }
    }

    if (result == &null_lisp_value) {
        return lisp_value_error_new(ERR_UNBOUND_SYMBOL_MESSAGE);
    }

    return result;
}

bool lisp_environment_exists(lisp_environment_t* env, char* symbol_str) {
    lisp_value_t* symbol = lisp_value_symbol_new(symbol_str);
    lisp_value_t* result = lisp_environment_get(env, symbol);
    lisp_value_delete(symbol);

    if (result == &null_lisp_value) {
        return false;
    }
    if (result->value_type == VAL_ERR && strcmp(result->error_message, ERR_UNBOUND_SYMBOL_MESSAGE) == 0) {
        return false;
    }
    return true;
}

bool is_lisp_environment_null(lisp_environment_t *env) {
    return env == &null_lisp_environment;
}

bool lisp_environment_setup_builtin_function(lisp_environment_t* env, char* symbol) {
    lisp_value_t* symbol_lisp_value = lisp_value_symbol_new(symbol);
    lisp_value_t* builtin_fun_lisp_value = lisp_value_builtin_fun_new(symbol);
    bool ok = lisp_environment_set(env, symbol_lisp_value, builtin_fun_lisp_value);
    lisp_value_delete(symbol_lisp_value);
    lisp_value_delete(builtin_fun_lisp_value);
    return ok;
}

bool lisp_environment_setup_builtin_functions(lisp_environment_t *env) {
    if (env == &null_lisp_environment) {
        return false;
    }

    bool ok = true;
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_PLUS);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_MINUS);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_MULTIPLY);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_DIVIDE);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_MOD);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_POW);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_MIN);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_MAX);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LIST);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_HEAD);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_TAIL);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_JOIN);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_EVAL);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_CONS);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LEN);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_INIT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_DEF);

    return ok;
}

void println_lisp_environment(lisp_environment_t* env) {
    puts("Lisp environment:");
    for (size_t i = 0; i < env->count; i++) {
        printf("%s\t", env->symbols[i]);
        print_lisp_value(env->values[i]);
        putchar('\n');
    }
}
