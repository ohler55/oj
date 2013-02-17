/* oj.h
 * Copyright (c) 2011, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * - Neither the name of Peter Ohler nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
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

#ifndef __OJ_H__
#define __OJ_H__

#if defined(__cplusplus)
extern "C" {
#if 0
} /* satisfy cc-mode */
#endif
#endif

#define RSTRING_NOT_MODIFIED

#include "ruby.h"
#if HAS_ENCODING_SUPPORT
#include "ruby/encoding.h"
#endif

#include "stdint.h"
#if SAFE_CACHE
#include <pthread.h>
#endif
#include "cache.h"

#ifdef RUBINIUS_RUBY
#undef T_RATIONAL
#undef T_COMPLEX
enum st_retval {ST_CONTINUE = 0, ST_STOP = 1, ST_DELETE = 2, ST_CHECK};
#else
#if HAS_TOP_LEVEL_ST_H
// Only on travis, local is where it is for all others. Seems to vary depending on the travis machine picked up.
#include "st.h"
#else
#include "ruby/st.h"
#endif
#endif

#define raise_error(msg, xml, current) _oj_raise_error(msg, xml, current, __FILE__, __LINE__)

#define MAX_ODD_ARGS	10

typedef enum {
    Yes	   = 'y',
    No	   = 'n',
    NotSet = 0
} YesNo;

typedef enum {
    StrictMode	= 's',
    ObjectMode	= 'o',
    NullMode	= 'n',
    CompatMode	= 'c'
} Mode;

typedef enum {
    UnixTime	= 'u',
    XmlTime	= 'x',
    RubyTime	= 'r'
} TimeFormat;

typedef struct _DumpOpts {
    const char	*indent;
    const char	*before_sep;
    const char	*after_sep;
    const char	*hash_nl;
    const char	*array_nl;
    uint8_t	indent_size;
    uint8_t	before_size;
    uint8_t	after_size;
    uint8_t	hash_size;
    uint8_t	array_size;
} *DumpOpts;

typedef struct _Options {
    int		indent;		// indention for dump, default 2
    char	circular;	// YesNo
    char	auto_define;	// YesNo
    char	sym_key;	// YesNo
    char	ascii_only;	// YesNo
    char	mode;		// Mode
    char	time_format;	// TimeFormat
    char	bigdec_as_num;	// YesNo
    const char	*create_id;	// 0 or string
    size_t	max_stack;	// max size to allocate on the stack
    int		sec_prec;	// second precision when dumping time
    DumpOpts	dump_opts;
} *Options;

typedef struct _Odd {
    VALUE	clas;			// Ruby class
    VALUE	create_obj;
    ID		create_op;
    int		attr_cnt;
    ID		attrs[MAX_ODD_ARGS];	// 0 terminated attr IDs
} *Odd;

enum {
    STR_VAL  = 0x00,
    COL_VAL  = 0x01,
    RUBY_VAL = 0x02
};
    
typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*key;	   // hash key
	size_t		index;	   // array index, 0 is not set
    };
    union {
	char		*str;	   // pointer to location in json string
	struct _Leaf	*elements; // array and hash elements
	VALUE		value;
    };
    uint8_t		type;
    uint8_t		parent_type;
    uint8_t		value_type;
} *Leaf;

extern VALUE	oj_parse(char *json, Options options);
extern void	oj_saj_parse(VALUE handler, char *json);

extern char*	oj_write_obj_to_str(VALUE obj, Options copts);
extern void	oj_write_obj_to_file(VALUE obj, const char *path, Options copts);
extern char*	oj_write_leaf_to_str(Leaf leaf, Options copts);
extern void	oj_write_leaf_to_file(Leaf leaf, const char *path, Options copts);

extern void	_oj_raise_error(const char *msg, const char *xml, const char *current, const char* file, int line);

extern void	oj_init_doc(void);

extern Odd	oj_get_odd(VALUE clas);

extern VALUE	Oj;
extern struct _Options	oj_default_options;
#if HAS_ENCODING_SUPPORT
extern rb_encoding	*oj_utf8_encoding;
#endif

extern VALUE	oj_bag_class;
extern VALUE	oj_bigdecimal_class;
extern VALUE	oj_date_class;
extern VALUE	oj_datetime_class;
extern VALUE	oj_doc_class;
extern VALUE	oj_parse_error_class;
extern VALUE	oj_stringio_class;
extern VALUE	oj_struct_class;
extern VALUE	oj_time_class;

extern VALUE	oj_slash_string;

extern ID	oj_add_value_id;
extern ID	oj_array_end_id;
extern ID	oj_array_start_id;
extern ID	oj_as_json_id;
extern ID	oj_error_id;
extern ID	oj_hash_end_id;
extern ID	oj_hash_start_id;
extern ID	oj_instance_variables_id;
extern ID	oj_json_create_id;
extern ID	oj_new_id;
extern ID	oj_string_id;
extern ID	oj_to_hash_id;
extern ID	oj_to_json_id;
extern ID	oj_to_s_id;
extern ID	oj_to_sym_id;
extern ID	oj_to_time_id;
extern ID	oj_tv_nsec_id;
extern ID	oj_tv_sec_id;
extern ID	oj_tv_usec_id;
extern ID	oj_utc_offset_id;

extern Cache	oj_class_cache;
extern Cache	oj_attr_cache;

#if SAFE_CACHE
extern pthread_mutex_t	oj_cache_mutex;
#endif
#if defined(__cplusplus)
#if 0
{ /* satisfy cc-mode */
#endif
}  /* extern "C" { */
#endif
#endif /* __OJ_H__ */
