#include "interpreter.h"

#include "mpc/mpc.h"

#include <math.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int MAX_ERROR_MESSAGE_BUFF_SIZE = 512;

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
static char* ERR_AT_EXACTLY_N_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE = "Expected exactly %d argument for %s";
static char* ERR_NOT_ALLOWED_TO_REDEFINE_BUILTIN_FUN_MESSAGE_TEMPLATE = "Builtin %s not allowed to be redefined";
static char* BOOLEAN_TYPE_MESSAGE = "VAL_NUMBER or VAL_BOOLEAN";

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
static char* BUILTIN_LOCAL_DEF = "=";
static char* BUILTIN_CREATE_FUNCTION = "\\";
static char* BUILTIN_DEF_FUN = "fun";
static char* BUILTIN_GT = ">";
static char* BUILTIN_GE = ">=";
static char* BUILTIN_LT = "<";
static char* BUILTIN_LE = "<=";
static char* BUILTIN_EQ = "==";
static char* BUILTIN_NE = "!=";
static char* BUILTIN_IF = "if";
static char* BUILTIN_OR = "||";
static char* BUILTIN_OR_OR = "or";
static char* BUILTIN_AND = "&&";
static char* BUILTIN_AND_AND = "and";
static char* BUILTIN_NOT = "!";
static char* BUILTIN_NOT_NOT = "not";
static char* BUILTIN_LOAD = "load";
static char* BUILTIN_PRINT = "print";
static char* BUILTIN_ERROR = "error";

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
    .values = NULL,
    .parent_environment = NULL
};

static lisp_environment_t lisp_environment_referenced_by_root_environment = {
    .symbols = NULL,
    .values = NULL,
    .parent_environment = NULL
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
    lisp_value->value_userdefined_fun = NULL;
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
        case VAL_USERDEFINED_FUN:
            return "Function";
        case VAL_BOOLEAN:
            return "Boolean";
        case VAL_STRING:
            return "String";
        default:
            return "Unknown type";
    }
}

bool has_tag(mpc_ast_t * ast, char * tag) {
    return strstr(ast->tag, tag) != NULL;
}

bool is_root(mpc_ast_t * ast) {
    return strcmp(ast->tag, ">") == 0;
}

/* Assumes ast is not NULL
 * Will return get_null_lisp_value() if the lisp_value is not parsed completely
 *  * if the ast does not represent the language that this function parses
 *  * if the program is not able to allocate lisp_value_t* objects
 */
lisp_value_t* parse_lisp_value(mpc_ast_t* ast) {
    if (has_tag(ast, "number")) {
        errno = 0;
        long number = strtol(ast->contents, NULL, 10);
        if (errno != 0) {
            return get_lisp_value_error_bad_numeric_value();
        }
        return lisp_value_number_new(number);
    }
    if (has_tag(ast, "boolean")) {
        int r = 0;
        if (strcmp("true", ast->contents) == 0) {
            r = 1;
        }
        return lisp_value_boolean_new(r);
    }
    if (has_tag(ast, "decimal")) {
        errno = 0;
        double number_decimal = strtod(ast->contents, NULL);
        if (errno != 0) {
            return get_lisp_value_error_bad_numeric_value();
        }
        return lisp_value_decimal_new(number_decimal);
    }
    if (has_tag(ast, "string")) {
        return lisp_value_string_new(ast->contents);
    }
    if (has_tag(ast, "symbol")) {
        return lisp_value_symbol_new(ast->contents);
    }

    lisp_value_t* lisp_value = get_null_lisp_value();
    if (is_root(ast)) {
        lisp_value = lisp_value_root_new();
    } else if (has_tag(ast, "sexpr")) {
        lisp_value = lisp_value_sexpr_new();
    } else if (has_tag(ast, "qexpr")) {
        lisp_value = lisp_value_qexpr_new();
    }

    if (is_lisp_value_null(lisp_value)) {
        return get_null_lisp_value();
    }

    int i = 1;
    while (has_tag(ast->children[i], "expr")) {
        if (has_tag(ast->children[i], "comment")) {
            i++;
            continue;
        }
        lisp_value_t* current_parsed_lisp_value = parse_lisp_value(ast->children[i]);
        if (is_lisp_value_null(current_parsed_lisp_value)) {
            lisp_value_delete(lisp_value);
            return get_null_lisp_value();
        }
        bool ok = append_lisp_value(lisp_value, current_parsed_lisp_value);
        if (!ok) {
            lisp_value_delete(current_parsed_lisp_value);
            lisp_value_delete(lisp_value);
            return get_null_lisp_value();
        }
        i++;
    }

    return lisp_value;
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
        if (lisp_value->values != NULL) {
            for (int i = 0; i < lisp_value->count; i++) {
                lisp_value_delete(lisp_value->values[i]);
            }
        }
        free(lisp_value->values);
    } else if (lisp_value->value_type == VAL_SYMBOL || lisp_value->value_type == VAL_BUILTIN_FUN) {
        free(lisp_value->value_symbol);
    } else if (lisp_value->value_type == VAL_ERR) {
        free(lisp_value->error_message);
    } else if (lisp_value->value_type == VAL_USERDEFINED_FUN) {
        if (lisp_value->value_userdefined_fun != NULL) {
            lisp_value_delete(lisp_value->value_userdefined_fun->formal_arguments);
            lisp_value_delete(lisp_value->value_userdefined_fun->varargs_symbol);
            lisp_value_delete(lisp_value->value_userdefined_fun->body);
            lisp_environment_delete(lisp_value->value_userdefined_fun->local_env);
        }
        free(lisp_value->value_userdefined_fun);
    } else if (lisp_value->value_type == VAL_STRING) {
        free(lisp_value->value_string);
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

lisp_value_t* lisp_value_userdefined_fun_new(lisp_environment_t* environment, lisp_value_t* formal_arguments, lisp_value_t* body) {
    if (formal_arguments->value_type != VAL_QEXPR || body->value_type != VAL_QEXPR) {
        lisp_value_delete(formal_arguments);
        lisp_value_delete(body);
        return lisp_value_error_new("error defining function");
    }

    bool are_all_formal_arguments_symbols = true;
    bool invalid_varargs_definiton = false;
    // TODO: try to use size_t where possible in situations like this??
    long varargs_signifier_index = -1;
    for (long i = 0; i < formal_arguments->count; i++) {
        if (formal_arguments->values[i]->value_type != VAL_SYMBOL) {
            are_all_formal_arguments_symbols = false;
            break;
        }
        if (strcmp(formal_arguments->values[i]->value_symbol, "&") == 0) {
            if (varargs_signifier_index != -1) {
                invalid_varargs_definiton = true;
                break;
            }
            varargs_signifier_index = i;
        }
    }
    if (!are_all_formal_arguments_symbols) {
        lisp_value_delete(formal_arguments);
        lisp_value_delete(body);
        return lisp_value_error_new("error defining function");
    }
    if ((varargs_signifier_index != -1 && varargs_signifier_index != formal_arguments->count - 2) || invalid_varargs_definiton) {
        lisp_value_delete(formal_arguments);
        lisp_value_delete(body);
        return lisp_value_error_new("error defining function: invalid varargs definition");
    }

    lisp_value_t* varargs_symbol = &null_lisp_value;
    if (varargs_signifier_index != -1) {
        lisp_value_delete(lisp_value_pop_child(formal_arguments, varargs_signifier_index));
        varargs_symbol = lisp_value_pop_child(formal_arguments, varargs_signifier_index);
    }

    lisp_value_t* lisp_value = lisp_value_new(VAL_USERDEFINED_FUN);
    // TODO: replace this(and in every occurence with check if lisp_value equals to null_lisp_value
    //  basically make lisp_value_new return null_lisp_value
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value_userdefined_fun_t* userdefined_fun = malloc(sizeof(lisp_value_userdefined_fun_t));
    if (userdefined_fun == NULL) {
        lisp_value_delete(lisp_value);
        return &null_lisp_value;
    }
    userdefined_fun->formal_arguments = formal_arguments;
    userdefined_fun->varargs_symbol = varargs_symbol;
    userdefined_fun->body = body;
    userdefined_fun->local_env = lisp_environment_new_with_parent(environment);
    lisp_value->value_userdefined_fun = userdefined_fun;
    if (userdefined_fun->local_env == &null_lisp_environment) {
        lisp_value_delete(lisp_value);
        return lisp_value_error_new("error defining function");
    }
    return lisp_value;
}

lisp_value_t* lisp_value_boolean_new(long value) {
    lisp_value_t* lisp_value = lisp_value_new(VAL_BOOLEAN);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }
    lisp_value->value_number = value;
    return lisp_value;
}

lisp_value_t* lisp_value_string_new(const char* value) {
    lisp_value_t* lisp_value = lisp_value_new(VAL_STRING);
    if (lisp_value == NULL) {
        return &null_lisp_value;
    }

    size_t value_buffer_size = strlen(value) + 1;
    size_t value_without_quotes_buffer_size = value_buffer_size - 2;
    char* value_without_quotes = malloc(value_without_quotes_buffer_size);
    if (value_without_quotes == NULL) {
        lisp_value_delete(lisp_value);
        return &null_lisp_value;
    }
    strncpy(value_without_quotes, value + 1, value_without_quotes_buffer_size - 1);
    value_without_quotes[value_without_quotes_buffer_size - 1] = '\0';
    lisp_value->value_string = mpcf_unescape(value_without_quotes);
    // we do not free value_without_quotes since mpcf_unescape frees it already
    if (lisp_value->value_string == NULL) {
        lisp_value_delete(lisp_value);
        return &null_lisp_value;
    }
    return lisp_value;
}

lisp_value_t* lisp_value_error_new(char* error_message_template, ...) {
    lisp_value_t* lisp_error = lisp_value_new(VAL_ERR);
    if (lisp_error == NULL) {
        return &null_lisp_value;
    }
    lisp_error->is_error_user_defined_value = 0;
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
    if (value == &null_lisp_value) {
        return &null_lisp_value;
    }

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
        case VAL_BOOLEAN:
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
            /*
             * Set malloc size to the next larger value divisible by 10,
             * for example, if count is 16, the next larger value divisible by 10 is 20
             *  This is in order to not break the implementation for appending a child to sexpr lisp_value_t*
             */
            copy->values = malloc(sizeof(lisp_value_t*) * (value->count + (10 - value->count % 10)));
            if (copy->values == NULL) {
                ok = false;
                break;
            }
            for (int i = 0; i < value->count; i++) {
                copy->values[i] = lisp_value_copy(value->values[i]);
                if (copy->values[i] == &null_lisp_value && value->values[i] != &null_lisp_value) {
                    ok = false;
                    break;
                }
            }
            break;
        case VAL_USERDEFINED_FUN:
            copy->value_userdefined_fun = malloc(sizeof(lisp_value_userdefined_fun_t));
            if (copy->value_userdefined_fun == NULL) {
                ok = false;
                break;
            }
            copy->value_userdefined_fun->local_env = lisp_environment_copy(value->value_userdefined_fun->local_env);
            copy->value_userdefined_fun->formal_arguments = lisp_value_copy(value->value_userdefined_fun->formal_arguments);
            copy->value_userdefined_fun->varargs_symbol = lisp_value_copy(value->value_userdefined_fun->varargs_symbol);
            copy->value_userdefined_fun->body = lisp_value_copy(value->value_userdefined_fun->body);
            if ((copy->value_userdefined_fun->local_env == &null_lisp_environment && value->value_userdefined_fun->local_env != &null_lisp_environment)
                || (copy->value_userdefined_fun->formal_arguments == &null_lisp_value && value->value_userdefined_fun->formal_arguments != &null_lisp_value)
                || (copy->value_userdefined_fun->body == &null_lisp_value && value->value_userdefined_fun->body != &null_lisp_value)) {
                ok = false;
            }
        break;
        case VAL_STRING:
            copy->value_string = malloc(strlen(value->value_string) + 1);
            if (copy->value_string == NULL) {
                ok = false;
                break;
            }
            strcpy(copy->value_string, value->value_string);
        break;
    }

    if (!ok) {
        lisp_value_delete(copy);
        return &null_lisp_value;
    }

    return copy;
}

int lisp_value_equals(lisp_value_t* first, lisp_value_t* second) {
    if (first == &null_lisp_value && second == &null_lisp_value) {
        return 1;
    }
    if (first == &null_lisp_value || second == &null_lisp_value) {
        return 0;
    }

    int r = 0;
    if (first->value_type == second->value_type) {
        switch (first->value_type) {
            case VAL_ERR:
                r = strcmp(first->error_message, second->error_message) == 0;
            break;
            case VAL_NUMBER:
            case VAL_BOOLEAN:
                r = first->value_number == second->value_number;
            break;
            case VAL_DECIMAL:
                r = first->value_decimal == second->value_decimal;
            break;
            case VAL_SYMBOL:
                r = strcmp(first->value_symbol, second->value_symbol) == 0;
            break;
            case VAL_SEXPR:
            case VAL_ROOT:
            case VAL_QEXPR:
                r = first->count == second->count;
                if (r == 0) {
                    break;
                }
                for (size_t i = 0; i < first->count && r != 0; i++) {
                    r = lisp_value_equals(first->values[i], second->values[i]);
                }
            break;
            case VAL_BUILTIN_FUN:
                r = strcmp(first->value_symbol, second->value_symbol) == 0;
            break;
            case VAL_USERDEFINED_FUN:
                int equals_formal_arguments = lisp_value_equals(first->value_userdefined_fun->formal_arguments, second->value_userdefined_fun->formal_arguments);
                int equals_body = lisp_value_equals(first->value_userdefined_fun->body, second->value_userdefined_fun->body);
                int equals_varargs_symbol = lisp_value_equals(first->value_userdefined_fun->varargs_symbol, second->value_userdefined_fun->varargs_symbol);
                r = equals_formal_arguments && equals_body && equals_varargs_symbol;
            break;
            default:
                r = 1;
        }
    }

    return r;
}

lisp_value_t* get_null_lisp_value() {
    return &null_lisp_value;
}

lisp_value_t* get_lisp_value_error_bad_numeric_value() {
    return lisp_value_error_new(ERR_BAD_NUMERIC_VALUE_MESSAGE);
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
            printf("%s", lisp_value->value_symbol);
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
        case VAL_USERDEFINED_FUN:
            printf("(\\ ");
            print_lisp_value(lisp_value->value_userdefined_fun->formal_arguments);
            printf(" ");
            print_lisp_value(lisp_value->value_userdefined_fun->body);
            printf(")");
        break;
        case VAL_BOOLEAN:
            if (lisp_value->value_number == 0) {
                printf("false");
            } else {
                printf("true");
            }
        break;
        case VAL_STRING:
            char* escaped_string = malloc(strlen(lisp_value->value_string) + 1);
            strcpy(escaped_string, lisp_value->value_string);
            escaped_string = mpcf_escape(escaped_string);
            printf("\"%s\"", escaped_string);
            free(escaped_string);
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
    if (is_lisp_value_error(value) && value->is_error_user_defined_value == 0) {
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

// TODO: change to accept one qexpr instead of a container of one qexpr
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
    lisp_environment_t* root_environment = NULL;
    while (env != NULL && env != &null_lisp_environment) {
        if (env->parent_environment == &lisp_environment_referenced_by_root_environment) {
            root_environment = env;
            break;
        }
        env = env->parent_environment;
    }

    // root_environment should never be null, we'll let the program crash by the principle of fail-fast
    // if(for some magical reason) root_environment is null
    return lisp_environment_put_variables(root_environment, arguments, BUILTIN_DEF);
}

lisp_value_t* builtin_local_def(lisp_environment_t* env, lisp_value_t* arguments) {
    return lisp_environment_put_variables(env, arguments, BUILTIN_LOCAL_DEF);
}

lisp_value_t* lisp_environment_put_variables(lisp_environment_t* env, lisp_value_t* arguments, char* function_name) {
    lisp_value_t* qexpr_of_symbols = lisp_value_pop_child(arguments, 0);
    if (qexpr_of_symbols->value_type != VAL_QEXPR) {
        lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, function_name, get_value_type_string(VAL_QEXPR), get_value_type_string(qexpr_of_symbols->value_type));
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
        // TODO: maybe change this a bit? We are checking if we are trying to overwrite a builtin
        //  maybe change it to be more efficient(i.e do not use lisp_environment_get since it copes and the copied value
        //  needs to be freed
        lisp_value_t* value = lisp_environment_get(env, qexpr_of_symbols->values[i]);
        if (value->value_type == VAL_BUILTIN_FUN) {
            lisp_value_t* error = lisp_value_error_new(ERR_NOT_ALLOWED_TO_REDEFINE_BUILTIN_FUN_MESSAGE_TEMPLATE, value->value_symbol);
            lisp_value_delete(value);
            lisp_value_delete(qexpr_of_symbols);
            lisp_value_delete(arguments);
            return error;
        }
        lisp_value_delete(value);
        lisp_environment_set(env, qexpr_of_symbols->values[i], arguments->values[i]);
    }

    lisp_value_delete(qexpr_of_symbols);
    lisp_value_delete(arguments);
    return lisp_value_sexpr_new();
}

lisp_value_t * builtin_create_function(lisp_environment_t * env, lisp_value_t * value) {
    if (value == &null_lisp_value) {
        return lisp_value_error_new("error defining function");
    }
    if (value->count != 2) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }
    if (value->values[0]->value_type != VAL_QEXPR || value->values[1]->value_type != VAL_QEXPR) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }

    lisp_value_t* userdefined_function = lisp_value_userdefined_fun_new(
        env,
        lisp_value_copy(value->values[0]),
        lisp_value_copy(value->values[1])
        );
    lisp_value_delete(value);
    return userdefined_function;
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

lisp_value_t* builtin_ordering_operation_for_numeric_arguments(char* operation, lisp_value_t* arguments) {
    if (arguments->count != 2) {
        lisp_value_t* error = lisp_value_error_new(ERR_AT_EXACTLY_N_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE, 2, operation);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->values[0]->value_type != arguments->values[1]->value_type) {
        lisp_value_t* error = lisp_value_error_new("arguments must be of same type");
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->values[0]->value_type != VAL_DECIMAL && arguments->values[0]->value_type != VAL_NUMBER) {
        lisp_value_t* error = lisp_value_error_new("arguments must be of numeric type(VAL_NUMBER or VAL_DECIMAL)");
        lisp_value_delete(arguments);
        return error;
    }

    lisp_value_type_t numeric_type = arguments->values[0]->value_type;

    int r = 0;
    if (strcmp(operation, ">") == 0) {
        if (numeric_type == VAL_NUMBER) {
            r = arguments->values[0]->value_number > arguments->values[1]->value_number;
        } else {
            r = arguments->values[0]->value_decimal > arguments->values[1]->value_decimal;
        }
    } else if (strcmp(operation, ">=") == 0) {
        if (numeric_type == VAL_NUMBER) {
            r = arguments->values[0]->value_number >= arguments->values[1]->value_number;
        } else {
            r = arguments->values[0]->value_decimal >= arguments->values[1]->value_decimal;
        }
    } else if (strcmp(operation, "<") == 0) {
        if (numeric_type == VAL_NUMBER) {
            r = arguments->values[0]->value_number < arguments->values[1]->value_number;
        } else {
            r = arguments->values[0]->value_decimal < arguments->values[1]->value_decimal;
        }
    } else if (strcmp(operation, "<=") == 0) {
        if (numeric_type == VAL_NUMBER) {
            r = arguments->values[0]->value_number <= arguments->values[1]->value_number;
        } else {
            r = arguments->values[0]->value_decimal <= arguments->values[1]->value_decimal;
        }
    }

    lisp_value_delete(arguments);
    return lisp_value_number_new(r);
}

lisp_value_t* builtin_eq(char* operation, lisp_value_t* arguments) {
    if (arguments->count != 2) {
        lisp_value_t* error = lisp_value_error_new(ERR_AT_EXACTLY_N_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE, 2, operation);
        lisp_value_delete(arguments);
        return error;
    }
    lisp_value_t* first = arguments->values[0];
    lisp_value_t* second = arguments->values[1];

    int r = lisp_value_equals(first, second);

    if (strcmp(operation, "!=") == 0) {
        r = !r;
    }
    lisp_value_delete(arguments);
    return lisp_value_boolean_new(r);
}

lisp_value_t * builtin_def_fun(lisp_environment_t * env, lisp_value_t * value) {
    if (value == &null_lisp_value) {
        return lisp_value_error_new("error defining function");
    }
    if (value->count != 2) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }
    if (value->values[0]->value_type != VAL_QEXPR || value->values[1]->value_type != VAL_QEXPR) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }
    if (value->values[0]->count < 1) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }

    lisp_value_t* function_name_symbol = lisp_value_pop_child(value->values[0], 0);
    if (function_name_symbol->value_type != VAL_SYMBOL) {
        lisp_value_delete(value);
        return lisp_value_error_new("error defining function");
    }

    lisp_value_t* function = lisp_value_userdefined_fun_new(
        env,
        lisp_value_copy(value->values[0]),
        lisp_value_copy(value->values[1])
        );

    lisp_value_delete(value);

    lisp_value_t* def_arguments = lisp_value_sexpr_new();
    if (def_arguments == &null_lisp_value) {
        return &null_lisp_value;
    }
    lisp_value_t* symbols = lisp_value_qexpr_new();
    if (symbols == &null_lisp_value) {
        return &null_lisp_value;
    }

    bool ok = append_lisp_value(symbols, function_name_symbol);
    ok = ok && append_lisp_value(def_arguments, symbols);
    ok = ok && append_lisp_value(def_arguments, function);

    if (!ok) {
        return &null_lisp_value;
    }

    return builtin_def(env, def_arguments);
}

lisp_value_t* builtin_if(lisp_environment_t* env, lisp_value_t* value) {
    if (value->count != 3) {
        lisp_value_delete(value);
        return lisp_value_error_new("if: required 3 arguments");
    }
    if (value->values[0]->value_type != VAL_NUMBER && value->values[0]->value_type != VAL_BOOLEAN) {
        lisp_value_delete(value);
        return lisp_value_error_new("if: argument 0 should be %s", BOOLEAN_TYPE_MESSAGE);
    }
    if (value->values[1]->value_type != VAL_QEXPR) {
        lisp_value_delete(value);
        return lisp_value_error_new("if: argument 1 should be VAL_QEXPR");
    }
    if (value->values[2]->value_type != VAL_QEXPR) {
        lisp_value_delete(value);
        return lisp_value_error_new("if: argument 1 should be VAL_QEXPR");
    }

    lisp_value_t* container_for_evaluation = lisp_value_qexpr_new();
    if (value->values[0]->value_number == 0) {
        append_lisp_value(container_for_evaluation, value->values[2]);
        // builtin_eval will destroy the body, so to ignore double free, we set the reference to &null_lisp_value
        // if we don't want to do this, we could pop this child
        value->values[2] = &null_lisp_value;
    } else {
        append_lisp_value(container_for_evaluation, value->values[1]);
        // builtin_eval will destroy the body, so to ignore double free, we set the reference to &null_lisp_value
        // if we don't want to do this, we could pop this child
        value->values[1] = &null_lisp_value;
    }
    lisp_value_t* result = builtin_eval(env, container_for_evaluation);
    lisp_value_delete(value);
    return result;
}

lisp_value_t* builtin_logical_operation(char* operation, lisp_value_t* arguments) {
    if (strcmp(operation, BUILTIN_NOT) == 0 || strcmp(operation, BUILTIN_NOT_NOT) == 0) {
        if (arguments->count != 1) {
            lisp_value_t* error = lisp_value_error_new(ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE, operation, 1, arguments->count);
            lisp_value_delete(arguments);
            return error;
        }
        if (arguments->values[0]->value_type != VAL_NUMBER && arguments->values[0]->value_type != VAL_BOOLEAN) {
            lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, operation, BOOLEAN_TYPE_MESSAGE, get_value_type_string(arguments->values[0]->value_type));
            lisp_value_delete(arguments);
            return error;
        }
        lisp_value_t* result = lisp_value_pop_child(arguments, 0);
        result->value_type = VAL_BOOLEAN;
        result->value_number = !result->value_number;
        lisp_value_delete(arguments);
        return result;
    } else {
        if (arguments->count == 0) {
            lisp_value_t* error = lisp_value_error_new(ERR_AT_LEAST_ONE_ARGUMENT_EXPECTED_MESSAGE_TEMPLATE, operation);
            lisp_value_delete(arguments);
            return error;
        }
        int r = strcmp(operation, BUILTIN_AND) == 0 || strcmp(operation, BUILTIN_AND_AND) == 0
        ? 1
        : 0;
        for (size_t i = 0; i < arguments->count; i++) {
            if (arguments->values[i]->value_type != VAL_NUMBER && arguments->values[i]->value_type != VAL_BOOLEAN) {
                lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, operation, get_value_type_string(VAL_NUMBER), get_value_type_string(arguments->values[0]->value_type));
                lisp_value_delete(arguments);
                return error;
            }
            if (strcmp(operation, BUILTIN_AND) == 0 || strcmp(operation, BUILTIN_AND_AND) == 0) {
                r = r && arguments->values[i]->value_number;
                if (r == 0) {
                    break;
                }
            } else {
                r = r || arguments->values[i]->value_number;
                if (r == 1) {
                    break;
                }
            }
        }
        lisp_value_delete(arguments);
        return lisp_value_boolean_new(r);
    }
}

lisp_value_t* builtin_load(char* operation, lisp_environment_t* env, lisp_value_t* arguments) {
    if (arguments->count != 1) {
        lisp_value_t* error = lisp_value_error_new(ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE, operation, 1, arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->values[0]->value_type != VAL_STRING) {
        lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, operation, get_value_type_string(VAL_STRING), get_value_type_string(arguments->values[0]->value_type));
        lisp_value_delete(arguments);
        return error;
    }

    lisp_environment_t* root_env = env;
    while (root_env->parent_environment != &lisp_environment_referenced_by_root_environment) {
        root_env = root_env->parent_environment;
    }

    mpc_result_t parse_result;
    if (parse_contents(arguments->values[0]->value_string, &parse_result)) {
        lisp_value_t* loaded_lisp_expressions = parse_lisp_value(parse_result.output);
        if (loaded_lisp_expressions->value_type == VAL_ROOT) {
            while (loaded_lisp_expressions->count > 0) {
                lisp_value_t* lisp_value = lisp_value_pop_child(loaded_lisp_expressions, 0);
                lisp_value_t* evaluated = evaluate_lisp_value_destructive(root_env, lisp_value);
                if (evaluated->value_type == VAL_ERR) {
                    print_lisp_value(evaluated);
                    putchar('\n');
                }
                lisp_value_delete(evaluated);
            }
        }
        lisp_value_delete(loaded_lisp_expressions);
        mpc_ast_delete(parse_result.output);
    } else {
        lisp_value_t* error = lisp_value_error_new(mpc_err_string(parse_result.error));
        lisp_value_delete(arguments);
        mpc_err_delete(parse_result.error);
        return error;
    }

    lisp_value_delete(arguments);
    return lisp_value_sexpr_new();
}

lisp_value_t* builtin_print(lisp_value_t* arguments) {
    if (!should_contain_children(arguments)) {
        lisp_value_delete(arguments);
        return lisp_value_error_new("builtin_print: need a list of values to print");
    }
    for (size_t i = 0; i < arguments->count; i++) {
        print_lisp_value(arguments->values[i]);
        if (i != arguments->count - 1) {
            putchar(' ');
        }
    }
    putchar('\n');
    lisp_value_delete(arguments);
    return lisp_value_sexpr_new();
}

lisp_value_t* builtin_error(lisp_value_t* arguments) {
    // TODO: make this type of checking reusable with macros maybe, similar to book's code?
    if (arguments->count != 1) {
        lisp_value_t* error = lisp_value_error_new(ERR_INVALID_NUMBER_OF_ARGUMENTS_MESSAGE_TEMPLATE, BUILTIN_ERROR, 1, arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->values[0]->value_type != VAL_STRING) {
        lisp_value_t* error = lisp_value_error_new(ERR_INCOMPATIBLE_TYPES_MESSAGE_TEMPLATE, 1, BUILTIN_ERROR, get_value_type_string(VAL_STRING), get_value_type_string(arguments->values[0]->value_type));
        lisp_value_delete(arguments);
        return error;
    }

    lisp_value_t* error_value = lisp_value_error_new(arguments->values[0]->value_string);
    if (error_value->value_type == VAL_ERR) {
        error_value->is_error_user_defined_value = 1;
    }
    lisp_value_delete(arguments);
    return error_value;
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

    if (strcmp(operation->value_symbol, BUILTIN_GT) == 0
        || strcmp(operation->value_symbol, BUILTIN_LT) == 0
        || strcmp(operation->value_symbol, BUILTIN_GE) == 0
        || strcmp(operation->value_symbol, BUILTIN_LE) == 0) {
        lisp_value_t* result = builtin_ordering_operation_for_numeric_arguments(operation->value_symbol, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_EQ) == 0 || strcmp(operation->value_symbol, BUILTIN_NE) == 0) {
        lisp_value_t* result = builtin_eq(operation->value_symbol, value);
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

    if (strcmp(operation->value_symbol, BUILTIN_LOCAL_DEF) == 0) {
        lisp_value_t* result = builtin_local_def(env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_CREATE_FUNCTION) == 0) {
        lisp_value_t* result = builtin_create_function(env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_DEF_FUN) == 0) {
        lisp_value_t* result = builtin_def_fun(env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_IF) == 0) {
        lisp_value_t* result = builtin_if(env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_OR) == 0 || strcmp(operation->value_symbol, BUILTIN_OR_OR) == 0
        || strcmp(operation->value_symbol, BUILTIN_AND) == 0 || strcmp(operation->value_symbol, BUILTIN_AND_AND) == 0
        || strcmp(operation->value_symbol, BUILTIN_NOT) == 0 || strcmp(operation->value_symbol, BUILTIN_NOT_NOT) == 0) {
        lisp_value_t* result = builtin_logical_operation(operation->value_symbol, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_LOAD) == 0) {
        lisp_value_t* result = builtin_load(operation->value_symbol, env, value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_PRINT) == 0) {
        lisp_value_t* result = builtin_print(value);
        lisp_value_delete(operation);
        return result;
    }

    if (strcmp(operation->value_symbol, BUILTIN_ERROR) == 0) {
        lisp_value_t* result = builtin_error(value);
        lisp_value_delete(operation);
        return result;
    }

    lisp_value_delete(operation);
    lisp_value_delete(value);
    return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
}

lisp_value_t* call_function_or_builtin_operation(lisp_environment_t* env, lisp_value_t* value) {
    lisp_value_t* operation = value->values[0];
    if (operation->value_type == VAL_BUILTIN_FUN) {
        return builtin_operation(env, value);
    } else if (operation->value_type == VAL_USERDEFINED_FUN) {
        // bind arguments
        lisp_value_t* function = lisp_value_pop_child(value, 0);
        lisp_value_t* formal_arguments = function->value_userdefined_fun->formal_arguments;
        size_t argument_values_count = value->count;
        size_t argument_symbols_count = formal_arguments->count;
        if (argument_values_count > argument_symbols_count && function->value_userdefined_fun->varargs_symbol == &null_lisp_value) {
            lisp_value_delete(function);
            lisp_value_delete(value);
            // TODO: move this (and others cases similar to this) to a constant
            return lisp_value_error_new("error evaluating function");
        }

        size_t min_size = argument_values_count < argument_symbols_count ? argument_values_count : argument_symbols_count;
        for (size_t i = 0; i < min_size; i++) {
            lisp_value_t* argument_symbol = lisp_value_pop_child(formal_arguments, 0);
            lisp_value_t* argument_value = lisp_value_pop_child(value, 0);
            lisp_environment_set(function->value_userdefined_fun->local_env, argument_symbol, argument_value);
            lisp_value_delete(argument_symbol);
        }

        if (argument_values_count < argument_symbols_count) {
            lisp_value_delete(value);
            return function;
        }

        if (function->value_userdefined_fun->varargs_symbol != &null_lisp_value) {
            lisp_value_t* varargs_qexpr = lisp_value_new(VAL_QEXPR);
            while (value->count > 0) {
                append_lisp_value(varargs_qexpr, lisp_value_pop_child(value, 0));
            }
            lisp_environment_set(function->value_userdefined_fun->local_env, function->value_userdefined_fun->varargs_symbol, varargs_qexpr);
        }

        function->value_userdefined_fun->local_env->parent_environment = env;

        lisp_value_t* container_of_body = lisp_value_new(VAL_QEXPR);
        append_lisp_value(container_of_body, function->value_userdefined_fun->body);
        lisp_value_t* result = builtin_eval(function->value_userdefined_fun->local_env, container_of_body);
        // builtin_eval will destroy the body, so to ignore double free, we set the reference to &null_lisp_value
        // if we don't want to do this, we could copy the body
        function->value_userdefined_fun->body = &null_lisp_value;
        lisp_value_delete(function);
        lisp_value_delete(value);
        return result;
    } else {
        lisp_value_delete(value);
        return lisp_value_error_new(ERR_INVALID_OPERATOR_MESSAGE);
    }
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
            if (is_lisp_value_error(value->values[i]) && value->values[i]->is_error_user_defined_value == 0) {
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
        return call_function_or_builtin_operation(env, value);
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
    env->parent_environment = NULL;

    return env;
}

lisp_environment_t* lisp_environment_new_root() {
    return lisp_environment_new_with_parent(&lisp_environment_referenced_by_root_environment);
}

lisp_environment_t* lisp_environment_new_with_parent(lisp_environment_t* parent_env) {
    lisp_environment_t* env = lisp_environment_new();
    if (env == &null_lisp_environment) {
        return &null_lisp_environment;
    }
    env->parent_environment = parent_env;
    return env;
}

lisp_environment_t* lisp_environment_copy(lisp_environment_t* env) {
    if (env == &null_lisp_environment) {
        return &null_lisp_environment;
    }

    lisp_environment_t* copy = malloc(sizeof(lisp_environment_t));
    if (copy == NULL) {
        return &null_lisp_environment;
    }

    copy->count = env->count;
    /**
     * Set malloc size to the next larger value divisible by 10,
     * for example, if count is 16, the next larger value divisible by 10 is 20
     * This is in order to not break the implementation for appending a child to sexpr lisp_value_t*
     */
    copy->symbols = malloc(sizeof(char*) * (env->count + (10 - env->count % 10)));
    copy->values = malloc(sizeof(lisp_value_t*) * (env->count + (10 - env->count % 10)));
    if (copy->symbols == NULL || copy->values == NULL) {
        lisp_environment_delete(copy);
        return &null_lisp_environment;
    }
    for (size_t i = 0; i < env->count; i++) {
        copy->symbols[i] = malloc(strlen(env->symbols[i]) + 1);
        if (copy->symbols[i] == NULL) {
            lisp_environment_delete(copy);
            return &null_lisp_environment;
        }
        strcpy(copy->symbols[i], env->symbols[i]);
        copy->values[i] = lisp_value_copy(env->values[i]);
        if (copy->values[i] == &null_lisp_value && env->values[i] != &null_lisp_value) {
            lisp_environment_delete(copy);
            return &null_lisp_environment;
        }
    }
    copy->parent_environment = env->parent_environment;
    return copy;
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
    if (env == &lisp_environment_referenced_by_root_environment) {
        return lisp_value_error_new(ERR_UNBOUND_SYMBOL_MESSAGE);
    }


    lisp_value_t* result = &null_lisp_value;
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(symbol->value_symbol, env->symbols[i]) == 0) {
            result = lisp_value_copy(env->values[i]);
            break;
        }
    }

    if (result == &null_lisp_value) {
        return lisp_environment_get(env->parent_environment, symbol);
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
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LOCAL_DEF);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_CREATE_FUNCTION);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_DEF_FUN);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_GT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_GE);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LE);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_EQ);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_NE);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_IF);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_OR);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_OR_OR);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_AND);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_AND_AND);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_NOT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_NOT_NOT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_LOAD);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_PRINT);
    ok = ok && lisp_environment_setup_builtin_function(env, BUILTIN_ERROR);

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

lisp_value_t* load_file(lisp_environment_t* env, const char* filename) {
    lisp_value_t* arguments = lisp_value_sexpr_new();
    if (arguments == &null_lisp_value) {
        return &null_lisp_value;
    }
    lisp_value_t* string_value = lisp_value_new(VAL_STRING);
    if (string_value == &null_lisp_value) {
        lisp_value_delete(arguments);
        return &null_lisp_value;
    }
    string_value->value_string = malloc(strlen(filename) + 1);
    if (string_value->value_string == NULL) {
        lisp_value_delete(arguments);
        lisp_value_delete(string_value);
        return &null_lisp_value;
    }
    strcpy(string_value->value_string, filename);
    append_lisp_value(arguments, string_value);
    return builtin_load(BUILTIN_LOAD, env, arguments);
}
