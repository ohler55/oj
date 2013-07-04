/* object.c
 * Copyright (c) 2012, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "oj.h"
#include "err.h"
#include "parse.h"
#include "resolve.h"
#include "hash.h"
#include "odd.h"
#include "encode.h"

inline static long
read_long(const char *str, size_t len) {
    long	n = 0;

    for (; 0 < len; str++, len--) {
	if ('0' <= *str && *str <= '9') {
	    n = n * 10 + (*str - '0');
	} else {
	    return -1;
	}
    }
    return n;
}

static VALUE
hash_key(ParseInfo pi, const char *key, size_t klen, char k1) {
    VALUE	rkey;

    if (':' == k1) {
	rkey = rb_str_new(key + 1, klen - 1);
	rkey = oj_encode(rkey);
	rkey = rb_funcall(rkey, oj_to_sym_id, 0);
    } else {
	rkey = rb_str_new(key, klen);
	rkey = oj_encode(rkey);
	if (Yes == pi->options.sym_key) {
	    rkey = rb_str_intern(rkey);
	}
    }
    return rkey;
}

static VALUE
str_to_value(ParseInfo pi, const char *str, size_t len, const char *orig) {
    VALUE	rstr = Qnil;

    if (':' == *orig && 0 < len) {
	rstr = rb_str_new(str + 1, len - 1);
	rstr = oj_encode(rstr);
	rstr = rb_funcall(rstr, oj_to_sym_id, 0);
    } else if (pi->circ_array && 3 <= len && '^' == *orig && 'r' == orig[1]) {
	long	i = read_long(str + 2, len - 2);

	if (0 > i) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a valid ID number");
	    return Qnil;
	}
	rstr = oj_circ_array_get(pi->circ_array, i);
    } else {
	rstr = rb_str_new(str, len);
	rstr = oj_encode(rstr);
    }
    return rstr;
}

static int
hat_cstr(ParseInfo pi, Val parent, const char *key, size_t klen, const char *str, size_t len) {
    if (2 == klen) {
	switch (key[1]) {
	case 'o': // object
	    {	// name2class sets and error if the class is not found or created
		VALUE	clas = oj_name2class(pi, str, len, Yes == pi->options.auto_define);

		if (Qundef != clas) {
		    parent->val = rb_obj_alloc(clas);
		}
	    }
	    break;
	case 'O': // odd object
	    {
		Odd	odd = oj_get_oddc(str, len);

		if (0 == odd) {
		    return 0;
		}
		parent->val = odd->clas;
		parent->odd_args = oj_odd_alloc_args(odd);
	    }
	    break;
	case 'm':
	    parent->val = rb_str_new(str + 1, len - 1);
	    parent->val = oj_encode(parent->val);
	    parent->val = rb_funcall(parent->val, oj_to_sym_id, 0);
	    break;
	case 's':
	    parent->val = rb_str_new(str, len);
	    parent->val = oj_encode(parent->val);
	    break;
	case 'c': // class
	    parent->val = oj_name2class(pi, str, len, Yes == pi->options.auto_define);
	    break;
	default:
	    return 0;
	    break;
	}
	return 1; // handled
    }
    return 0;
}

static int
hat_num(ParseInfo pi, Val parent, const char *key, size_t klen, NumInfo ni) {
    if (2 == klen) {
	switch (key[1]) {
	case 't': // time as a float
	    {
		int64_t	nsec = ni->num * 1000000000LL / ni->div;

		if (ni->neg) {
		    ni->i = -ni->i;
		    if (0 < nsec) {
			ni->i--;
			nsec = 1000000000LL - nsec;
		    }
		}
#if HAS_NANO_TIME
		parent->val = rb_time_nano_new(ni->i, (long)nsec);
#else
		parent->val = rb_time_new(ni->i, (long)(nsec / 1000));
#endif
	    }
	    break;
	case 'i': // circular index
	    if (!ni->infinity && !ni->neg && 1 == ni->div && 0 == ni->exp && 0 != pi->circ_array) { // fixnum
		if (Qnil == parent->val) {
		    parent->val = rb_hash_new();
		}
		oj_circ_array_set(pi->circ_array, parent->val, ni->i);
	    } else {
		return 0;
	    }
	    break;
	default:
	    return 0;
	    break;
	}
	return 1; // handled
    }
    return 0;
}

static int
hat_value(ParseInfo pi, Val parent, const char *key, size_t klen, VALUE value) {
    if (2 == klen && 'u' == key[1] && T_ARRAY == rb_type(value)) {
#if HAS_RSTRUCT
	long	len = RARRAY_LEN(value);
	VALUE	*a = RARRAY_PTR(value);
	VALUE	sc;
	VALUE	s;
	VALUE	*sv;

	if (0 == len) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "Invalid struct data");
	    return 1;
	}
	sc = rb_const_get(oj_struct_class, rb_to_id(*a));
	//sc = rb_const_get(oj_struct_class, rb_intern_str(*a));
	// use encoding as the indicator for Ruby 1.8.7 or 1.9.x
#if HAS_ENCODING_SUPPORT
	s = rb_struct_alloc_noinit(sc);
#else
	s = rb_struct_new(sc);
#endif
	sv = RSTRUCT_PTR(s);
	if (RSTRUCT_LEN(s) < len - 1) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "Too many elements for Struct");
	    return 1;
	}
	for (a++; 0 < len; len--, a++, sv++) {
	    *sv = *a;
	}
	parent->val = s;
#else
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "Ruby structs not supported with this version of Ruby");
#endif
	return 1;
    } else if (3 <= klen && '#' == key[1] && T_ARRAY == rb_type(value)) {
	long	len = RARRAY_LEN(value);
	VALUE	*a = RARRAY_PTR(value);
	
	if (2 != len) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "invalid hash pair");
	    return 1;
	}
	parent->val = rb_hash_new();
	rb_hash_aset(parent->val, *a, a[1]);
	return 1;
    }
    return 0;
}

static void
copy_ivars(VALUE target, VALUE src) {
    VALUE	vars = rb_funcall(src, oj_instance_variables_id, 0);
    VALUE	*np = RARRAY_PTR(vars);
    ID		vid;
    int		i, cnt = (int)RARRAY_LEN(vars);
    const char	*attr;

    for (i = cnt; 0 < i; i--, np++) {
	vid = rb_to_id(*np);
	attr = rb_id2name(vid);
	if ('@' == *attr) {
	    rb_ivar_set(target, vid, rb_ivar_get(src, vid));
	}
    }
}

static void
set_obj_ivar(Val parent, const char *key, size_t klen, VALUE value) {
    ID	var_id;
    ID	*slot;

    if ('~' == *key && Qtrue == rb_obj_is_kind_of(parent->val, rb_eException)) {
	if (5 == klen && 0 == strncmp("~mesg", key, klen)) {
	    VALUE	args[1];
	    VALUE	prev = parent->val;

	    args[0] = value;
	    parent->val = rb_class_new_instance(1, args, rb_class_of(parent->val));
	    copy_ivars(parent->val, prev);
	} else if (3 == klen && 0 == strncmp("~bt", key, klen)) {
	    rb_funcall(parent->val, rb_intern("set_backtrace"), 1, value);
	}
    }
#if SAFE_CACHE
    pthread_mutex_lock(&oj_cache_mutex);
#endif
    if (0 == (var_id = oj_attr_hash_get(key, klen, &slot))) {
	char	attr[256];

	if (sizeof(attr) <= klen + 2) {
	    char	*buf = ALLOC_N(char, klen + 2);

	    if ('~' == *key) {
		strncpy(buf, key + 1, klen - 1);
		buf[klen - 1] = '\0';
	    } else {
		*buf = '@';
		strncpy(buf + 1, key, klen);
		buf[klen + 1] = '\0';
	    }
	    var_id = rb_intern(buf);
	    xfree(buf);
	} else {
	    if ('~' == *key) {
		strncpy(attr, key + 1, klen - 1);
		attr[klen - 1] = '\0';
	    } else {
		*attr = '@';
		strncpy(attr + 1, key, klen);
		attr[klen + 1] = '\0';
	    }
	    var_id = rb_intern(attr);
	}
	*slot = var_id;
    }
#if SAFE_CACHE
    pthread_mutex_unlock(&oj_cache_mutex);
#endif
    rb_ivar_set(parent->val, var_id, value);
}

static void
hash_set_cstr(ParseInfo pi, const char *key, size_t klen, const char *str, size_t len, const char *orig) {
    Val	parent = stack_peek(&pi->stack);

 WHICH_TYPE:
    switch (rb_type(parent->val)) {
    case T_NIL:
	parent->odd_args = 0; // make sure it is 0 in case not odd
	if ('^' != *key || !hat_cstr(pi, parent, key, klen, str, len)) {
	    parent->val = rb_hash_new();
	    goto WHICH_TYPE;
	}
	break;
    case T_HASH:
	rb_hash_aset(parent->val, hash_key(pi, key, klen, parent->k1), str_to_value(pi, str, len, orig));
	break;
    case T_OBJECT:
	set_obj_ivar(parent, key, klen, str_to_value(pi, str, len, orig));
	break;
    case T_CLASS:
	if (0 == parent->odd_args) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an odd class", rb_class2name(rb_obj_class(parent->val)));
	    return;
	} else if (0 != oj_odd_set_arg(parent->odd_args, key, klen, str_to_value(pi, str, len, orig))) {
	    char	buf[256];

	    if (sizeof(buf) - 1 <= klen) {
		klen = sizeof(buf) - 2;
	    }
	    memcpy(buf, key, klen);
	    buf[klen] = '\0';
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an attribute of %s", buf, rb_class2name(rb_obj_class(parent->val)));
	}
	break;
    default:
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "can not add attributes to a %s", rb_class2name(rb_obj_class(parent->val)));
	return;
    }
}

static void
hash_set_num(ParseInfo pi, const char *key, size_t klen, NumInfo ni) {
    Val	parent = stack_peek(&pi->stack);

 WHICH_TYPE:
    switch (rb_type(parent->val)) {
    case T_NIL:
	parent->odd_args = 0; // make sure it is 0 in case not odd
	if ('^' != *key || !hat_num(pi, parent, key, klen, ni)) {
	    parent->val = rb_hash_new();
	    goto WHICH_TYPE;
	}
	break;
    case T_HASH:
	rb_hash_aset(parent->val, hash_key(pi, key, klen, parent->k1), oj_num_as_value(ni));
	break;
    case T_OBJECT:
	if (2 == klen && '^' == *key && 'i' == key[1] &&
	    !ni->infinity && !ni->neg && 1 == ni->div && 0 == ni->exp && 0 != pi->circ_array) { // fixnum
	    oj_circ_array_set(pi->circ_array, parent->val, ni->i);
	} else {
	    set_obj_ivar(parent, key, klen, oj_num_as_value(ni));
	}
	break;
    case T_CLASS:
	if (0 == parent->odd_args) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an odd class", rb_class2name(rb_obj_class(parent->val)));
	    return;
	} else if (0 != oj_odd_set_arg(parent->odd_args, key, klen, oj_num_as_value(ni))) {
	    char	buf[256];

	    if (sizeof(buf) - 1 <= klen) {
		klen = sizeof(buf) - 2;
	    }
	    memcpy(buf, key, klen);
	    buf[klen] = '\0';
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an attribute of %s", buf, rb_class2name(rb_obj_class(parent->val)));
	}
	break;
    default:
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "can not add attributes to a %s", rb_class2name(rb_obj_class(parent->val)));
	return;
    }
}

static void
hash_set_value(ParseInfo pi, const char *key, size_t klen, VALUE value) {
    Val	parent = stack_peek(&pi->stack);

 WHICH_TYPE:
    switch (rb_type(parent->val)) {
    case T_NIL:
	parent->odd_args = 0; // make sure it is 0 in case not odd
	if ('^' != *key || !hat_value(pi, parent, key, klen, value)) {
	    parent->val = rb_hash_new();
	    goto WHICH_TYPE;
	}
	break;
    case T_HASH:
	if (3 <= klen && '#' == key[1] && T_ARRAY == rb_type(value)) {
	    long	len = RARRAY_LEN(value);
	    VALUE	*a = RARRAY_PTR(value);
	
	    if (2 != len) {
		oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "invalid hash pair");
		return;
	    }
	    rb_hash_aset(parent->val, *a, a[1]);
	} else {
	    rb_hash_aset(parent->val, hash_key(pi, key, klen, parent->k1), value);
	}
	break;
    case T_OBJECT:
	set_obj_ivar(parent, key, klen, value);
	break;
    case T_CLASS:
	if (0 == parent->odd_args) {
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an odd class", rb_class2name(rb_obj_class(parent->val)));
	    return;
	} else if (0 !=	oj_odd_set_arg(parent->odd_args, key, klen, value)) {
	    char	buf[256];

	    if (sizeof(buf) - 1 <= klen) {
		klen = sizeof(buf) - 2;
	    }
	    memcpy(buf, key, klen);
	    buf[klen] = '\0';
	    oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "%s is not an attribute of %s", buf, rb_class2name(rb_obj_class(parent->val)));
	}
	break;
    default:
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "can not add attributes to a %s", rb_class2name(rb_obj_class(parent->val)));
	return;
    }
}


static VALUE
start_hash(ParseInfo pi) {
    return Qnil;
}

static void
end_hash(struct _ParseInfo *pi) {
    Val	parent = stack_peek(&pi->stack);

    if (Qnil == parent->val) {
	parent->val = rb_hash_new();
    } else if (0 != parent->odd_args) {
	OddArgs	oa = parent->odd_args;

	parent->val = rb_funcall2(oa->odd->create_obj, oa->odd->create_op, oa->odd->attr_cnt, oa->args);
	oj_odd_free(oa);
	parent->odd_args = 0;
    }
}

static void
array_append_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    if (3 <= len && 0 != pi->circ_array) {
	if ('i' == str[1]) {
	    long	i = read_long(str + 2, len - 2);

	    if (0 < i) {
		oj_circ_array_set(pi->circ_array, stack_peek(&pi->stack)->val, i);
		return;
	    }
	} else if ('r' == str[1]) {
	    long	i = read_long(str + 2, len - 2);

	    if (0 < i) {
		rb_ary_push(stack_peek(&pi->stack)->val, oj_circ_array_get(pi->circ_array, i));
		return;
	    }
	    
	}
    }
    rb_ary_push(stack_peek(&pi->stack)->val, str_to_value(pi, str, len, orig));
}

static void
add_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    pi->stack.head->val = str_to_value(pi, str, len, orig);
}

VALUE
oj_object_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_strict_callbacks(&pi);
    pi.end_hash = end_hash;
    pi.start_hash = start_hash;
    pi.hash_set_cstr = hash_set_cstr;
    pi.hash_set_num = hash_set_num;
    pi.hash_set_value = hash_set_value;
    pi.add_cstr = add_cstr;
    pi.array_append_cstr = array_append_cstr;

    return oj_pi_parse(argc, argv, &pi, 0);
}

VALUE
oj_object_parse_cstr(int argc, VALUE *argv, char *json) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_strict_callbacks(&pi);
    pi.end_hash = end_hash;
    pi.start_hash = start_hash;
    pi.hash_set_cstr = hash_set_cstr;
    pi.hash_set_num = hash_set_num;
    pi.hash_set_value = hash_set_value;
    pi.add_cstr = add_cstr;
    pi.array_append_cstr = array_append_cstr;

    return oj_pi_parse(argc, argv, &pi, json);
}
