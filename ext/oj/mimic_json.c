/* mimic_json.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "oj.h"
#include "encode.h"
#include "dump.h"
#include "parse.h"

static VALUE	create_additions_sym;
static VALUE	symbolize_names_sym;

static const char	json_class[] = "json_class";

VALUE	oj_array_nl_sym;
VALUE	oj_ascii_only_sym;
VALUE	oj_json_parser_error_class;
VALUE	oj_max_nesting_sym;
VALUE	oj_object_nl_sym;
VALUE	oj_space_before_sym;
VALUE	oj_space_sym;

// mimic JSON documentation

/* Document-module: JSON::Ext
 * 
 * The Ext module is a placeholder in the mimic JSON module used for
 * compatibility only.
 */
/* Document-class: JSON::Ext::Parser
 * 
 * The JSON::Ext::Parser is a placeholder in the mimic JSON module used for
 * compatibility only.
 */
/* Document-class: JSON::Ext::Generator
 * 
 * The JSON::Ext::Generator is a placeholder in the mimic JSON module used for
 * compatibility only.
 */

/* Document-method: parser=
 * @overload parser=(parser) -> nil
 * 
 * Does nothing other than provide compatibiltiy.
 * @param parser [Object] ignored
 */
/* Document-method: generator=
 * @overload generator=(generator) -> nil
 * 
 * Does nothing other than provide compatibiltiy.
 * @param generator [Object] ignored
 */


/* Document-module: JSON
 * @!method dump(obj, anIO=nil, limit = nil)
 * 
 * Encodes an object as a JSON String.
 * 
 * @param obj [Object] object to convert to encode as JSON
 * @param anIO [IO] an IO that allows writing
 * @param limit [Fixnum] ignored
 * @return [String]
 */
static VALUE
mimic_dump(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;
    
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    out.omit_nil = copts.dump_opts.omit_nil;
    oj_dump_obj_to_json(*argv, &copts, &out);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (2 <= argc && Qnil != argv[1]) {
	VALUE	io = argv[1];
	VALUE	args[1];

	*args = rstr;
	rb_funcall2(io, oj_write_id, 1, args);
	rstr = io;
    }
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}

// This is the signature for the hash_foreach callback also.
static int
mimic_walk(VALUE key, VALUE obj, VALUE proc) {
    switch (rb_type(obj)) {
    case T_HASH:
	rb_hash_foreach(obj, mimic_walk, proc);
	break;
    case T_ARRAY:
	{
	    size_t	cnt = RARRAY_LEN(obj);
	    size_t	i;

	    for (i = 0; i < cnt; i++) {
		mimic_walk(Qnil, rb_ary_entry(obj, i), proc);
	    }
	    break;
	}
    default:
	break;
    }
    if (Qnil == proc) {
	if (rb_block_given_p()) {
	    rb_yield(obj);
	}
    } else {
#if HAS_PROC_WITH_BLOCK
	VALUE	args[1];

	*args = obj;
	rb_proc_call_with_block(proc, 1, args, Qnil);
#else
	rb_raise(rb_eNotImpError, "Calling a Proc with a block not supported in this version. Use func() {|x| } syntax instead.");
#endif
    }
    return ST_CONTINUE;
}

/* Document-module: JSON
 * @!method restore(source, proc=nil)
 * 
 * Loads a Ruby Object from a JSON source that can be either a String or an
 * IO. If Proc is given or a block is providedit is called with each nested
 * element of the loaded Object.
 * 
 * @param source [String|IO] JSON source
 * @param proc [Proc] to yield to on each element or nil
 * @return [Object}
 */

/* Document-module: JSON
 * @!method load(source, proc=nil)
 * 
 * Loads a Ruby Object from a JSON source that can be either a String or an
 * IO. If Proc is given or a block is providedit is called with each nested
 * element of the loaded Object.
 * 
 * @param source [String|IO] JSON source
 * @param proc [Proc] to yield to on each element or nil
 * @return [Object}
 */
static VALUE
mimic_load(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;
    VALUE		obj;
    VALUE		p = Qnil;

    pi.err_class = oj_json_parser_error_class;
    pi.options = oj_default_options;
    oj_set_compat_callbacks(&pi);

    obj = oj_pi_parse(argc, argv, &pi, 0, 0, 0);
    if (2 <= argc) {
	p = argv[1];
    }
    mimic_walk(Qnil, obj, p);

    return obj;
}

/* Document-module: JSON
 * @!method [](obj, opts={})
 * 
 * If the obj argument is a String then it is assumed to be a JSON String and
 * parsed otherwise the obj is encoded as a JSON String.
 * 
 * @param obj [String|Hash|Array] object to convert
 * @param opts [Hash] same options as either generate or parse
 * @return [Object]
 */
static VALUE
mimic_dump_load(int argc, VALUE *argv, VALUE self) {
    if (1 > argc) {
	rb_raise(rb_eArgError, "wrong number of arguments (0 for 1)");
    } else if (T_STRING == rb_type(*argv)) {
	return mimic_load(argc, argv, self);
    } else {
	return mimic_dump(argc, argv, self);
    }
    return Qnil;
}

static VALUE
mimic_generate_core(int argc, VALUE *argv, Options copts) {
    char	buf[4096];
    struct _Out	out;
    VALUE	rstr;
    
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    out.omit_nil = copts->dump_opts.omit_nil;
    // For obj.to_json or generate nan is not allowed but if called from dump
    // it is.
    copts->dump_opts.nan_dump = false;
    copts->mode = CompatMode;
    if (2 == argc && Qnil != argv[1]) {
	VALUE	ropts = argv[1];
	VALUE	v;
	size_t	len;

	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_max_nesting_sym))) {
	    if (Qtrue == v) {
		copts->dump_opts.max_depth = 100;
	    } else {
		copts->dump_opts.max_depth = MAX_DEPTH;
	    }
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_allow_nan_sym))) {
	    copts->dump_opts.nan_dump = (Qtrue == v);
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_indent_sym))) { // TBD fixnum also ok
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.indent_str) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "indent string is limited to %lu characters.", sizeof(copts->dump_opts.indent_str));
	    }
	    strcpy(copts->dump_opts.indent_str, StringValuePtr(v));
	    copts->dump_opts.indent_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_space_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.after_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "space string is limited to %lu characters.", sizeof(copts->dump_opts.after_sep));
	    }
	    strcpy(copts->dump_opts.after_sep, StringValuePtr(v));
	    copts->dump_opts.after_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_space_before_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.before_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "space_before string is limited to %lu characters.", sizeof(copts->dump_opts.before_sep));
	    }
	    strcpy(copts->dump_opts.before_sep, StringValuePtr(v));
	    copts->dump_opts.before_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_object_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.hash_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "object_nl string is limited to %lu characters.", sizeof(copts->dump_opts.hash_nl));
	    }
	    strcpy(copts->dump_opts.hash_nl, StringValuePtr(v));
	    copts->dump_opts.hash_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_array_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.array_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "array_nl string is limited to %lu characters.", sizeof(copts->dump_opts.array_nl));
	    }
	    strcpy(copts->dump_opts.array_nl, StringValuePtr(v));
	    copts->dump_opts.array_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, oj_ascii_only_sym))) {
	    // generate seems to assume anything except nil and false are true.
	    if (Qfalse == v || Qnil == v) {
		copts->escape_mode = JSONEsc;
	    } else {
		copts->escape_mode = ASCIIEsc;
	    }
	}
    }
    oj_dump_obj_to_json(*argv, copts, &out);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}

/* Document-module: JSON
 * @!method fast_generate(obj, opts=nil)
 * Same as generate().
 * @see generate
 */

/* Document-module: JSON
 * @!method generate(obj, opts=nil)
 * 
 * Encode obj as a JSON String. The obj argument must be a Hash, Array, or
 * respond to to_h or to_json. Options other than those listed such as
 * +:allow_nan+ or +:max_nesting+ are ignored.
 * 
 * @param obj [Object|Hash|Array] object to convert to a JSON String
 * @param opts [Hash] options
 * @option opts [String] :indent String to use for indentation
 * @option opts [String] :space String placed after a , or : delimiter
 * @option opts_before [String] :space  String placed before a : delimiter
 * @option opts [String] :object_nl String placed after a JSON object
 * @option opts [String] :array_nl String placed after a JSON array
 * @option opts [Boolean] :ascii_only if not nil or false then use only ascii
 *                      characters in the output. Note JSON.generate does
 *                      support this even if it is not documented.
 * @return [String]
 */
VALUE
oj_mimic_generate(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;

    return mimic_generate_core(argc, argv, &copts);
}

/* Document-module: JSON
 * @!method pretty_generate(obj, opts=nil) -> String
 * Same as generate() but with different defaults for the spacing options.
 * @see generate
 * @return [String]
 */
VALUE
oj_mimic_pretty_generate(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;

    strcpy(copts.dump_opts.indent_str, "  ");
    copts.dump_opts.indent_size = (uint8_t)strlen(copts.dump_opts.indent_str);
    strcpy(copts.dump_opts.before_sep, "");
    copts.dump_opts.before_size = (uint8_t)strlen(copts.dump_opts.before_sep);
    strcpy(copts.dump_opts.after_sep, " ");
    copts.dump_opts.after_size = (uint8_t)strlen(copts.dump_opts.after_sep);
    strcpy(copts.dump_opts.hash_nl, "\n");
    copts.dump_opts.hash_size = (uint8_t)strlen(copts.dump_opts.hash_nl);
    strcpy(copts.dump_opts.array_nl, "\n");
    copts.dump_opts.array_size = (uint8_t)strlen(copts.dump_opts.array_nl);
    copts.dump_opts.use = true;

    return mimic_generate_core(argc, argv, &copts);
}

/* Document-module: JSON
 * @!method parse!(source, opts=nil)
 *
 * Same as parse().
 * @see parse
 */
/* Document-module: JSON
 * @!method parse(source, opts=nil)
 *
 * Parses a JSON String or IO into a Ruby Object.  Options other than those
 * listed such as +:allow_nan+ or +:max_nesting+ are ignored. +:object_class+ and
 * +:array_object+ are not supported.
 *
 * @param source [String|IO] source to parse
 * @param opts [Hash] options
 * @option opts [Boolean] :symbolize _names flag indicating JSON object keys should be Symbols instead of Strings
 * @option opts [Boolean] :create_additions flag indicating a key matching +create_id+ in a JSON object should trigger the creation of Ruby Object
 * @return [Object]
 * @see create_id=
 */
static VALUE
mimic_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;
    VALUE		args[1];

    if (argc < 1) {
	rb_raise(rb_eArgError, "Wrong number of arguments to parse.");
    }
    oj_set_compat_callbacks(&pi);
    pi.err_class = oj_json_parser_error_class;
    pi.options = oj_default_options;
    pi.options.auto_define = No;
    pi.options.quirks_mode = No;
    pi.options.allow_invalid = No;
    pi.options.empty_string = No;

    if (2 <= argc) {
	VALUE	ropts = argv[1];
	VALUE	v;

	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, symbolize_names_sym))) {
	    pi.options.sym_key = (Qtrue == v) ? Yes : No;
	}

	if (Qnil != (v = rb_hash_lookup(ropts, oj_quirks_mode_sym))) {
	    pi.options.quirks_mode = (Qtrue == v) ? Yes : No;
	}

	if (Qnil != (v = rb_hash_lookup(ropts, create_additions_sym))) {
	    if (Qfalse == v) {
		oj_set_strict_callbacks(&pi);
	    }
	}
	// :allow_nan is not supported as Oj always allows nan
	// :object_class is always Hash
	// :array_class is always Array
    }
    *args = *argv;

    return oj_pi_parse(1, args, &pi, 0, 0, 0);
}

/* Document-module: JSON
 * @!methof recurse_proc(obj, &proc)
 * 
 * Yields to the proc for every element in the obj recursivly.
 * 
 * @param obj [Hash|Array] object to walk
 * @param proc [Proc] to yield to on each element
 * @return [nil]
 */
static VALUE
mimic_recurse_proc(VALUE self, VALUE obj) {
    rb_need_block();
    mimic_walk(Qnil, obj, Qnil);

    return Qnil;
}

static VALUE
no_op1(VALUE self, VALUE obj) {
    return Qnil;
}

/* Document-module: JSON
 * @!method create_id=(id) -> String
 *
 * Sets the create_id tag to look for in JSON document. That key triggers the
 * creation of a class with the same name.
 *
 * @param id [nil|String] new create_id
 * @return [String] the id
 */
static VALUE
mimic_set_create_id(VALUE self, VALUE id) {
    Check_Type(id, T_STRING);

    if (0 != oj_default_options.create_id) {
	if (json_class != oj_default_options.create_id) {
	    xfree((char*)oj_default_options.create_id);
	}
	oj_default_options.create_id = 0;
	oj_default_options.create_id_len = 0;
    }
    if (Qnil != id) {
	size_t	len = RSTRING_LEN(id) + 1;

	oj_default_options.create_id = ALLOC_N(char, len);
	strcpy((char*)oj_default_options.create_id, StringValuePtr(id));
	oj_default_options.create_id_len = len - 1;
    }
    return id;
}

/* Document-module: JSON
 * @!method create_id() -> String
 *
 * @return [String] the create_id.
 */
static VALUE
mimic_create_id(VALUE self) {
    if (0 != oj_default_options.create_id) {
	return oj_encode(rb_str_new_cstr(oj_default_options.create_id));
    }
    return rb_str_new_cstr(json_class);
}

static struct _Options	mimic_object_to_json_options = {
    0,		// indent
    No,		// circular
    No,		// auto_define
    No,		// sym_key
    JXEsc,	// escape_mode
    CompatMode,	// mode
    No,		// class_cache
    RubyTime,	// time_format
    No,		// bigdec_as_num
    FloatDec,	// bigdec_load
    No,		// to_json
    Yes,	// as_json
    Yes,	// nilnil
    Yes,	// empty_string
    Yes,	// allow_gc
    Yes,	// quirks_mode
    No,		// allow_invalid
    json_class,	// create_id
    10,		// create_id_len
    3,		// sec_prec
    16,		// float_prec
    "%0.15g",	// float_fmt
    Qnil,	// hash_class
    {		// dump_opts
	false,	//use
	"",	// indent
	"",	// before_sep
	"",	// after_sep
	"",	// hash_nl
	"",	// array_nl
	0,	// indent_size
	0,	// before_size
	0,	// after_size
	0,	// hash_size
	0,	// array_size
	AutoNan,// nan_dump
	false,	// omit_nil
    }
};

static VALUE
mimic_object_to_json(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    VALUE		rstr;
    struct _Options	copts = oj_default_options;

    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    out.omit_nil = copts.dump_opts.omit_nil;
    copts.mode = CompatMode;
    copts.to_json = Yes;
    // To be strict the mimic_object_to_json_options should be used but people
    // seem to prefer the option of changing that.
    //oj_dump_obj_to_json(self, &mimic_object_to_json_options, &out);
    oj_dump_obj_to_json_using_params(self, &copts, &out, argc, argv);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}


/* Document-method: mimic_JSON
 * @overload mimic_JSON() => Module
 *
 * Creates the JSON module with methods and classes to mimic the JSON gem. After
 * this method is invoked calls that expect the JSON module will use Oj instead
 * and be faster than the original JSON. Most options that could be passed to
 * the JSON methods are supported. The calls to set parser or generator will not
 * raise an Exception but will not have any effect. The method can also be
 * called after the json gem is loaded. The necessary methods on the json gem
 * will be replaced with Oj methods.
 *
 * Note that this also sets the default options of :mode to :compat and
 * :encoding to :ascii.
 */
VALUE
oj_define_mimic_json(int argc, VALUE *argv, VALUE self) {
    VALUE	ext;
    VALUE	dummy;
    VALUE	verbose;
    VALUE	json_error;
    VALUE	mimic;
    
    // Either set the paths to indicate JSON has been loaded or replaces the
    // methods if it has been loaded.
    if (rb_const_defined_at(rb_cObject, rb_intern("JSON"))) {
	mimic = rb_const_get_at(rb_cObject, rb_intern("JSON"));
    } else {
	mimic = rb_define_module("JSON");
    }
    verbose = rb_gv_get("$VERBOSE");
    rb_gv_set("$VERBOSE", Qfalse);
    rb_define_module_function(rb_cObject, "JSON", mimic_dump_load, -1);
    if (rb_const_defined_at(mimic, rb_intern("Ext"))) {
	ext = rb_const_get_at(mimic, rb_intern("Ext"));
     } else {
	ext = rb_define_module_under(mimic, "Ext");
    }
    if (!rb_const_defined_at(ext, rb_intern("Parser"))) {
	dummy = rb_define_class_under(ext, "Parser", rb_cObject);
    }
    if (!rb_const_defined_at(ext, rb_intern("Generator"))) {
	dummy = rb_define_class_under(ext, "Generator", rb_cObject);
    }
    // convince Ruby that the json gem has already been loaded
    dummy = rb_gv_get("$LOADED_FEATURES");
    if (rb_type(dummy) == T_ARRAY) {
	rb_ary_push(dummy, rb_str_new2("json"));
	if (0 < argc) {
	    VALUE	mimic_args[1];

	    *mimic_args = *argv;
	    rb_funcall2(Oj, rb_intern("mimic_loaded"), 1, mimic_args);
	} else {
	    rb_funcall2(Oj, rb_intern("mimic_loaded"), 0, 0);
	}
    }
    rb_define_module_function(mimic, "parser=", no_op1, 1);
    rb_define_module_function(mimic, "generator=", no_op1, 1);
    rb_define_module_function(mimic, "create_id=", mimic_set_create_id, 1);
    rb_define_module_function(mimic, "create_id", mimic_create_id, 0);

    rb_define_module_function(mimic, "dump", mimic_dump, -1);
    rb_define_module_function(mimic, "load", mimic_load, -1);
    rb_define_module_function(mimic, "restore", mimic_load, -1);
    rb_define_module_function(mimic, "recurse_proc", mimic_recurse_proc, 1);
    rb_define_module_function(mimic, "[]", mimic_dump_load, -1);

    rb_define_module_function(mimic, "generate", oj_mimic_generate, -1);
    rb_define_module_function(mimic, "fast_generate", oj_mimic_generate, -1);
    rb_define_module_function(mimic, "pretty_generate", oj_mimic_pretty_generate, -1);
    /* for older versions of JSON, the deprecated unparse methods */
    rb_define_module_function(mimic, "unparse", oj_mimic_generate, -1);
    rb_define_module_function(mimic, "fast_unparse", oj_mimic_generate, -1);
    rb_define_module_function(mimic, "pretty_unparse", oj_mimic_pretty_generate, -1);

    rb_define_module_function(mimic, "parse", mimic_parse, -1);
    rb_define_module_function(mimic, "parse!", mimic_parse, -1);

    rb_define_method(rb_cObject, "to_json", mimic_object_to_json, -1);

    rb_gv_set("$VERBOSE", verbose);

    create_additions_sym = ID2SYM(rb_intern("create_additions"));	rb_gc_register_address(&create_additions_sym);
    symbolize_names_sym = ID2SYM(rb_intern("symbolize_names"));		rb_gc_register_address(&symbolize_names_sym);

    if (rb_const_defined_at(mimic, rb_intern("JSONError"))) {
        json_error = rb_const_get(mimic, rb_intern("JSONError"));
    } else {
        json_error = rb_define_class_under(mimic, "JSONError", rb_eStandardError);
    }
    if (rb_const_defined_at(mimic, rb_intern("ParserError"))) {
        oj_json_parser_error_class = rb_const_get(mimic, rb_intern("ParserError"));
    } else {
    	oj_json_parser_error_class = rb_define_class_under(mimic, "ParserError", json_error);
    }
    if (!rb_const_defined_at(mimic, rb_intern("State"))) {
        rb_define_class_under(mimic, "State", rb_cObject);
    }

    oj_default_options = mimic_object_to_json_options;
    oj_default_options.to_json = Yes;

    return mimic;
}