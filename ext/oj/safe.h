#include <ruby.h>

#include "parser.h"
#include "usual.h"

#define SET_CONFIG(config_name)                                                        \
    do {                                                                               \
        VALUE rb_##config_name = rb_hash_aref(options, config_name##_sym);             \
                                                                                       \
        if (RB_INTEGER_TYPE_P(rb_##config_name)) {                                     \
            safe->config_name = NUM2LONG(rb_##config_name);                            \
        } else if (!NIL_P(rb_##config_name)) {                                         \
            rb_raise(rb_eArgError, "Incorrect value provided for `" #config_name "`"); \
        } else {                                                                       \
            safe->config_name = Qnil;                                                  \
        }                                                                              \
    } while (0);

#define DEFINE_DELEGATED_FUNCTION(function_name)   \
    static void safe_##function_name(ojParser p) { \
        safe_T safe = (safe_T)p->ctx;              \
                                                   \
        safe->current_elements_count++;            \
                                                   \
        check_array_size(safe);                    \
        check_max_total_elements(safe);            \
                                                   \
        safe->delegated_##function_name##_func(p); \
    }

#define DEFINE_DELEGATED_OBJECT_FUNCTION(function_name)  \
    static void safe_##function_name##_key(ojParser p) { \
        safe_T safe = (safe_T)p->ctx;                    \
                                                         \
        safe->current_elements_count += 2;               \
                                                         \
        check_object_size(safe);                         \
        check_max_total_elements(safe);                  \
                                                         \
        safe->delegated_##function_name##_key_func(p);   \
    }

typedef struct _safe_S {
    struct _usual usual;

    long int max_hash_size;
    long int max_array_size;
    long int max_depth;
    long int max_total_elements;
    long int max_json_size_bytes;

    long int current_hash_size;
    long int current_array_size;
    long int current_elements_count;

    void (*delegated_start_func)(struct _ojParser *p);

    // Array functions
    void (*delegated_open_object_func)(struct _ojParser *p);
    void (*delegated_open_array_func)(struct _ojParser *p);
    void (*delegated_add_null_func)(struct _ojParser *p);
    void (*delegated_add_true_func)(struct _ojParser *p);
    void (*delegated_add_false_func)(struct _ojParser *p);
    void (*delegated_add_int_func)(struct _ojParser *p);
    void (*delegated_add_float_func)(struct _ojParser *p);
    void (*delegated_add_big_func)(struct _ojParser *p);
    void (*delegated_add_str_func)(struct _ojParser *p);

    // Object functions
    void (*delegated_open_object_key_func)(struct _ojParser *p);
    void (*delegated_open_array_key_func)(struct _ojParser *p);
    void (*delegated_add_null_key_func)(struct _ojParser *p);
    void (*delegated_add_true_key_func)(struct _ojParser *p);
    void (*delegated_add_false_key_func)(struct _ojParser *p);
    void (*delegated_add_int_key_func)(struct _ojParser *p);
    void (*delegated_add_float_key_func)(struct _ojParser *p);
    void (*delegated_add_big_key_func)(struct _ojParser *p);
    void (*delegated_add_str_key_func)(struct _ojParser *p);
} *safe_T;
