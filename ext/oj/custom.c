/* custom.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include <stdio.h>

#include "oj.h"
#include "err.h"
#include "hash.h"
#include "parse.h"
#include "resolve.h"
#include "encode.h"

extern void	oj_set_obj_ivar(Val parent, Val kval, VALUE value);

static void
hash_set_cstr(ParseInfo pi, Val kval, const char *str, size_t len, const char *orig) {
    const char		*key = kval->key;
    int			klen = kval->klen;
    Val			parent = stack_peek(&pi->stack);
    volatile VALUE	rkey = kval->key_val;

    if (Qundef == rkey &&
	Yes == pi->options.create_ok &&
	NULL != pi->options.create_id &&
	*pi->options.create_id == *key &&
	(int)pi->options.create_id_len == klen &&
	0 == strncmp(pi->options.create_id, key, klen)) {

	parent->clas = oj_name2class(pi, str, len, false, rb_eArgError);
	if (2 == klen && '^' == *key && 'o' == key[1]) {
	    // TBD check class not in code
	    // oj_code_has(codes, parent->clas)

	    if (Qundef != parent->clas) {
		parent->val = rb_obj_alloc(parent->clas);
	    }
	}
    } else {
	volatile VALUE	rstr = rb_str_new(str, len);

	if (Qundef == rkey) {
	    rkey = rb_str_new(key, klen);
	    rstr = oj_encode(rstr);
	    rkey = oj_encode(rkey);
	    if (Yes == pi->options.sym_key) {
		rkey = rb_str_intern(rkey);
	    }
	}
	if (Yes == pi->options.create_ok && NULL != pi->options.str_rx.head) {
	    VALUE	clas = oj_rxclass_match(&pi->options.str_rx, str, len);

	    if (Qnil != clas) {
		rstr = rb_funcall(clas, oj_json_create_id, 1, rstr);
	    }
	}
	if (T_OBJECT == rb_type(parent->val)) {
	    oj_set_obj_ivar(parent, kval, rstr);
	} else {
	    rb_hash_aset(parent->val, rkey, rstr);
	}
    }
}

static void
end_hash(struct _ParseInfo *pi) {
    Val	parent = stack_peek(&pi->stack);

    if (Qundef != parent->clas && parent->clas != rb_obj_class(parent->val)) {
	parent->val = rb_funcall(parent->clas, oj_json_create_id, 1, parent->val);
	parent->clas = Qundef;
    }
}

static VALUE
calc_hash_key(ParseInfo pi, Val parent) {
    volatile VALUE	rkey = parent->key_val;

    if (Qundef == rkey) {
	rkey = rb_str_new(parent->key, parent->klen);
    }
    rkey = oj_encode(rkey);
    if (Yes == pi->options.sym_key) {
	rkey = rb_str_intern(rkey);
    }
    return rkey;
}

static void
hash_set_num(struct _ParseInfo *pi, Val kval, NumInfo ni) {
    Val	parent = stack_peek(&pi->stack);

    if (T_OBJECT == rb_type(parent->val)) {
	oj_set_obj_ivar(parent, kval, oj_num_as_value(ni));
    } else {
	rb_hash_aset(parent->val, calc_hash_key(pi, kval), oj_num_as_value(ni));
    }
}

static void
hash_set_value(ParseInfo pi, Val kval, VALUE value) {
    Val	parent = stack_peek(&pi->stack);

    if (T_OBJECT == rb_type(parent->val)) {
	oj_set_obj_ivar(parent, kval, value);
    } else {
	rb_hash_aset(parent->val, calc_hash_key(pi, kval), value);
    }
}

static void
array_append_num(ParseInfo pi, NumInfo ni) {
    Val	parent = stack_peek(&pi->stack);
    
    rb_ary_push(parent->val, oj_num_as_value(ni));
}

static void
array_append_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    volatile VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    if (Yes == pi->options.create_ok && NULL != pi->options.str_rx.head) {
	VALUE	clas = oj_rxclass_match(&pi->options.str_rx, str, len);

	if (Qnil != clas) {
	    rb_ary_push(stack_peek(&pi->stack)->val, rb_funcall(clas, oj_json_create_id, 1, rstr));
	    return;
	}
    }
    rb_ary_push(stack_peek(&pi->stack)->val, rstr);
}

void
oj_set_custom_callbacks(ParseInfo pi) {
    oj_set_compat_callbacks(pi);
    pi->hash_set_cstr = hash_set_cstr;
    pi->end_hash = end_hash;
    pi->hash_set_num = hash_set_num;
    pi->hash_set_value = hash_set_value;
    pi->array_append_cstr = array_append_cstr;
    pi->array_append_num = array_append_num;
}

VALUE
oj_custom_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;

    parse_info_init(&pi);
    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    pi.max_depth = 0;
    pi.options.allow_nan = Yes;
    pi.options.nilnil = Yes;
    oj_set_custom_callbacks(&pi);

    if (T_STRING == rb_type(*argv)) {
	return oj_pi_parse(argc, argv, &pi, 0, 0, false);
    } else {
	return oj_pi_sparse(argc, argv, &pi, 0);
    }
}

VALUE
oj_custom_parse_cstr(int argc, VALUE *argv, char *json, size_t len) {
    struct _ParseInfo	pi;

    parse_info_init(&pi);
    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    pi.max_depth = 0;
    pi.options.allow_nan = Yes;
    pi.options.nilnil = Yes;
    oj_set_custom_callbacks(&pi);
    pi.end_hash = end_hash;

    return oj_pi_parse(argc, argv, &pi, json, len, false);
}
