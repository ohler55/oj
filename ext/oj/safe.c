#include "safe.h"

static VALUE max_hash_size_sym, max_array_size_sym, max_depth_sym, max_total_elements_sym, max_hash_size_error_class,
    max_array_size_error_class, max_depth_error_class, max_total_elements_error_class;

static void check_object_size(safe_T safe) {
    if (NIL_P(safe->max_hash_size)) {
        return;
    }

    struct _usual usual                   = safe->usual;
    Col           current_object_location = usual.ctail - 1;

    long int number_of_items_in_stack = usual.vtail - usual.vhead;
    long int number_of_items_in_hash  = (number_of_items_in_stack - current_object_location->vi - 1) / 2;

    if (safe->max_hash_size > number_of_items_in_hash) {
        return;
    }

    rb_raise(max_hash_size_error_class, "Too many object items!");
}

static void check_array_size(safe_T safe) {
    if (NIL_P(safe->max_array_size)) {
        return;
    }

    struct _usual usual                   = safe->usual;
    Col           current_object_location = usual.ctail - 1;

    long int number_of_items_in_stack = usual.vtail - usual.vhead;
    long int number_of_items_in_array = number_of_items_in_stack - current_object_location->vi - 1;

    if (safe->max_array_size > number_of_items_in_array) {
        return;
    }

    rb_raise(max_array_size_error_class, "Too many array items!");
}

static void check_max_depth(safe_T safe, ojParser p) {
    if (NIL_P(safe->max_depth) || safe->max_depth >= (p->depth + 1)) {
        return;
    }

    rb_raise(max_depth_error_class, "JSON is too deep!");
}

static void check_max_total_elements(safe_T safe) {
    /*
    * We check if `max_total_elements` is greater than `current_elements_count`
    * (instead of greater than or equal) because top-level elements (e.g., [],
    * null, true) are not counted. As a result, `current_elements_count`
    * always holds one less than the actual total.
    */
    if (NIL_P(safe->max_total_elements) || safe->max_total_elements > safe->current_elements_count) {
        return;
    }

    rb_raise(max_total_elements_error_class, "Too many elements!");
}

static void safe_start(ojParser p) {
    safe_T safe = (safe_T)p->ctx;

    safe->current_hash_size      = 0;
    safe->current_array_size     = 0;
    safe->current_elements_count = 0;

    safe->delegated_start_func(p);
}

static void safe_open_object(ojParser p) {
    safe_T safe = (safe_T)p->ctx;

    safe->current_hash_size++;
    safe->current_elements_count++;

    check_array_size(safe);
    check_max_depth(safe, p);
    check_max_total_elements(safe);

    safe->delegated_open_object_func(p);
}

static void safe_open_array(ojParser p) {
    safe_T safe = (safe_T)p->ctx;

    safe->current_array_size++;
    safe->current_elements_count++;

    check_array_size(safe);
    check_max_depth(safe, p);
    check_max_total_elements(safe);

    safe->delegated_open_array_func(p);
}

DEFINE_DELEGATED_FUNCTION(add_null);
DEFINE_DELEGATED_FUNCTION(add_true);
DEFINE_DELEGATED_FUNCTION(add_false);
DEFINE_DELEGATED_FUNCTION(add_int);
DEFINE_DELEGATED_FUNCTION(add_float);
DEFINE_DELEGATED_FUNCTION(add_big);
DEFINE_DELEGATED_FUNCTION(add_str);

static void safe_open_object_key(ojParser p) {
    safe_T safe = (safe_T)p->ctx;

    safe->current_hash_size++;
    safe->current_elements_count += 2;

    check_object_size(safe);
    check_max_depth(safe, p);
    check_max_total_elements(safe);

    safe->delegated_open_object_key_func(p);
}

static void safe_open_array_key(ojParser p) {
    safe_T safe = (safe_T)p->ctx;

    safe->current_array_size++;
    safe->current_elements_count += 2;

    check_object_size(safe);
    check_max_depth(safe, p);
    check_max_total_elements(safe);

    safe->delegated_open_array_key_func(p);
}

DEFINE_DELEGATED_OBJECT_FUNCTION(add_null);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_true);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_false);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_int);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_float);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_big);
DEFINE_DELEGATED_OBJECT_FUNCTION(add_str);

void oj_init_safe_parser(ojParser p, safe_T safe, VALUE options) {
    // Safe parser inherits all members of usual parser
    oj_init_usual(p, &safe->usual);

    safe->delegated_start_func = p->start;
    p->start                   = safe_start;

    Funcs f;

    // Array parser functions
    f                                = &p->funcs[ARRAY_FUN];
    safe->delegated_open_object_func = f->open_object;
    f->open_object                   = safe_open_object;
    safe->delegated_open_array_func  = f->open_array;
    f->open_array                    = safe_open_array;
    // The following overrides are done for counting objects
    safe->delegated_add_null_func  = f->add_null;
    f->add_null                    = safe_add_null;
    safe->delegated_add_true_func  = f->add_true;
    f->add_true                    = safe_add_true;
    safe->delegated_add_false_func = f->add_false;
    f->add_false                   = safe_add_false;
    safe->delegated_add_int_func   = f->add_int;
    f->add_int                     = safe_add_int;
    safe->delegated_add_float_func = f->add_float;
    f->add_float                   = safe_add_float;
    safe->delegated_add_big_func   = f->add_big;
    f->add_big                     = safe_add_big;
    safe->delegated_add_str_func   = f->add_str;
    f->add_str                     = safe_add_str;

    // Object parser functions
    f                                    = &p->funcs[OBJECT_FUN];
    safe->delegated_open_object_key_func = f->open_object;
    f->open_object                       = safe_open_object_key;
    safe->delegated_open_array_key_func  = f->open_array;
    f->open_array                        = safe_open_array_key;
    // The following overrides are done for counting objects
    safe->delegated_add_null_key_func  = f->add_null;
    f->add_null                        = safe_add_null_key;
    safe->delegated_add_true_key_func  = f->add_true;
    f->add_true                        = safe_add_true_key;
    safe->delegated_add_false_key_func = f->add_false;
    f->add_false                       = safe_add_false_key;
    safe->delegated_add_int_key_func   = f->add_int;
    f->add_int                         = safe_add_int_key;
    safe->delegated_add_float_key_func = f->add_float;
    f->add_float                       = safe_add_float_key;
    safe->delegated_add_big_key_func   = f->add_big;
    f->add_big                         = safe_add_big_key;
    safe->delegated_add_str_key_func   = f->add_str;
    f->add_str                         = safe_add_str_key;

    SET_CONFIG(max_hash_size);
    SET_CONFIG(max_array_size);
    SET_CONFIG(max_depth);
    SET_CONFIG(max_total_elements);
}

void oj_set_parser_safe(ojParser p, VALUE options) {
    safe_T s = OJ_R_ALLOC(struct _safe_S);

    oj_init_safe_parser(p, s, options);
}

void oj_safe_init(VALUE parser_class) {
    VALUE validation_error_class = rb_define_class_under(parser_class, "ValidationError", rb_eRuntimeError);

    max_hash_size_error_class      = rb_define_class_under(parser_class, "HashSizeError", validation_error_class);
    max_array_size_error_class     = rb_define_class_under(parser_class, "ArraySizeError", validation_error_class);
    max_depth_error_class          = rb_define_class_under(parser_class, "DepthError", validation_error_class);
    max_total_elements_error_class = rb_define_class_under(parser_class, "TotalElementsError", validation_error_class);

    rb_gc_register_address(&max_hash_size_error_class);
    rb_gc_register_address(&max_array_size_error_class);
    rb_gc_register_address(&max_depth_error_class);
    rb_gc_register_address(&max_total_elements_error_class);

    max_hash_size_sym      = ID2SYM(rb_intern("max_hash_size"));
    max_array_size_sym     = ID2SYM(rb_intern("max_array_size"));
    max_depth_sym          = ID2SYM(rb_intern("max_depth"));
    max_total_elements_sym = ID2SYM(rb_intern("max_total_elements"));

    rb_gc_register_address(&max_hash_size_sym);
    rb_gc_register_address(&max_array_size_sym);
    rb_gc_register_address(&max_depth_sym);
    rb_gc_register_address(&max_total_elements_sym);
}
