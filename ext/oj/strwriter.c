/* dump_strict.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"

static void
key_check(StrWriter sw, const char *key) {
    DumpType	type = sw->types[sw->depth];

    if (0 == key && (ObjectNew == type || ObjectType == type)) {
	rb_raise(rb_eStandardError, "Can not push onto an Object without a key.");
    }
}

static void
push_type(StrWriter sw, DumpType type) {
    if (sw->types_end <= sw->types + sw->depth + 1) {
	size_t	size = (sw->types_end - sw->types) * 2;

	REALLOC_N(sw->types, char, size);
	sw->types_end = sw->types + size;
    }
    sw->depth++;
    sw->types[sw->depth] = type;
}

static void
maybe_comma(StrWriter sw) {
    switch (sw->types[sw->depth]) {
    case ObjectNew:
	sw->types[sw->depth] = ObjectType;
	break;
    case ArrayNew:
	sw->types[sw->depth] = ArrayType;
	break;
    case ObjectType:
    case ArrayType:
	// Always have a few characters available in the out.buf.
	*sw->out.cur++ = ',';
	break;
    }
}

void
oj_str_writer_push_key(StrWriter sw, const char *key) {
    DumpType	type = sw->types[sw->depth];
    long	size;

    if (sw->keyWritten) {
	rb_raise(rb_eStandardError, "Can not push more than one key before pushing a non-key.");
    }
    if (ObjectNew != type && ObjectType != type) {
	rb_raise(rb_eStandardError, "Can only push a key onto an Object.");
    }
    size = sw->depth * sw->out.indent + 3;
    assure_size(&sw->out, size);
    maybe_comma(sw);
    if (0 < sw->depth) {
	fill_indent(&sw->out, sw->depth);
    }
    oj_dump_cstr(key, strlen(key), 0, 0, &sw->out);
    *sw->out.cur++ = ':';
    sw->keyWritten = 1;
}

void
oj_str_writer_push_object(StrWriter sw, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
	assure_size(&sw->out, 1);
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	assure_size(&sw->out, size);
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    oj_dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    *sw->out.cur++ = '{';
    push_type(sw, ObjectNew);
}

void
oj_str_writer_push_array(StrWriter sw, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
	assure_size(&sw->out, 1);
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	assure_size(&sw->out, size);
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    oj_dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    *sw->out.cur++ = '[';
    push_type(sw, ArrayNew);
}

void
oj_str_writer_push_value(StrWriter sw, VALUE val, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	assure_size(&sw->out, size);
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    oj_dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    oj_dump_val(val, sw->depth, &sw->out, 0, 0, true);
}

void
oj_str_writer_push_json(StrWriter sw, const char *json, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	assure_size(&sw->out, size);
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    oj_dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    oj_dump_raw(json, strlen(json), &sw->out);
}

void
oj_str_writer_pop(StrWriter sw) {
    long	size;
    DumpType	type = sw->types[sw->depth];

    if (sw->keyWritten) {
	sw->keyWritten = 0;
	rb_raise(rb_eStandardError, "Can not pop after writing a key but no value.");
    }
    sw->depth--;
    if (0 > sw->depth) {
	rb_raise(rb_eStandardError, "Can not pop with no open array or object.");
    }
    size = sw->depth * sw->out.indent + 2;
    assure_size(&sw->out, size);
    fill_indent(&sw->out, sw->depth);
    switch (type) {
    case ObjectNew:
    case ObjectType:
	*sw->out.cur++ = '}';
	break;
    case ArrayNew:
    case ArrayType:
	*sw->out.cur++ = ']';
	break;
    }
    if (0 == sw->depth && 0 <= sw->out.indent) {
	*sw->out.cur++ = '\n';
    }
}

void
oj_str_writer_pop_all(StrWriter sw) {
    while (0 < sw->depth) {
	oj_str_writer_pop(sw);
    }
}
