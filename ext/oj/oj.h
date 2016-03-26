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
#include "stdbool.h"

#if USE_PTHREAD_MUTEX
#include <pthread.h>
#endif
#include "cache8.h"

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

#include "err.h"

#define INF_VAL		"3.0e14159265358979323846"
#define NINF_VAL	"-3.0e14159265358979323846"
#define NAN_VAL		"3.3e14159265358979323846"

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
    UnixZTime	= 'z',
    XmlTime	= 'x',
    RubyTime	= 'r'
} TimeFormat;

typedef enum {
    NLEsc	= 'n',
    JSONEsc	= 'j',
    XSSEsc	= 'x',
    ASCIIEsc	= 'a'
} Encoding;

typedef enum {
    BigDec	= 'b',
    FloatDec	= 'f',
    AutoDec	= 'a'
} BigLoad;

typedef enum {
    ArrayNew	= 'A',
    ArrayType	= 'a',
    ObjectNew	= 'O',
    ObjectType	= 'o',
} DumpType;

typedef enum {
    AutoNan	= 'a',
    NullNan	= 'n',
    HugeNan	= 'h',
    WordNan	= 'w',
    RaiseNan	= 'r',
} NanDump;

typedef enum {
    STRING_IO	= 'c',
    STREAM_IO	= 's',
    FILE_IO	= 'f',
} StreamWriterType;

typedef struct _DumpOpts {
    bool	use;
    char	indent_str[16];
    char	before_sep[16];
    char	after_sep[16];
    char	hash_nl[16];
    char	array_nl[16];
    uint8_t	indent_size;
    uint8_t	before_size;
    uint8_t	after_size;
    uint8_t	hash_size;
    uint8_t	array_size;
    char	nan_dump;	// NanDump
} *DumpOpts;

typedef struct _Options {
    int			indent;		// indention for dump, default 2
    char		circular;	// YesNo
    char		auto_define;	// YesNo
    char		sym_key;	// YesNo
    char		escape_mode;	// Escape_Mode
    char		mode;		// Mode
    char		class_cache;	// YesNo
    char		time_format;	// TimeFormat
    char		bigdec_as_num;	// YesNo
    char		bigdec_load;	// BigLoad
    char		to_json;	// YesNo
    char		nilnil;		// YesNo
    char		allow_gc;	// allow GC during parse
    char		quirks_mode;	// allow single JSON values instead of documents
    const char		*create_id;	// 0 or string
    size_t		create_id_len;	// length of create_id
    int			sec_prec;	// second precision when dumping time
    char		float_prec;	// float precision, linked to float_fmt
    char		float_fmt[7];	// float format for dumping, if empty use Ruby
    struct _DumpOpts	dump_opts;
} *Options;

typedef struct _Out {
    char	*buf;
    char	*end;
    char	*cur;
    Cache8	circ_cache;
    slot_t	circ_cnt;
    int		indent;
    int		depth; // used by dump_hash
    Options	opts;
    uint32_t	hash_cnt;
    int		allocated;
} *Out;

typedef struct _StrWriter {
    struct _Out		out;
    struct _Options	opts;
    int			depth;
    char		*types;	// DumpType
    char		*types_end;
    int			keyWritten;
} *StrWriter;

typedef struct _StreamWriter {
    struct _StrWriter	sw;
    StreamWriterType	type;
    VALUE		stream;
    int			fd;
} *StreamWriter;

enum {
    NO_VAL   = 0x00,
    STR_VAL  = 0x01,
    COL_VAL  = 0x02,
    RUBY_VAL = 0x03
};
    
typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*key;	   // hash key
	size_t		index;	   // array index, 0 is not set
    };
    union {
	char		*str;	   // pointer to location in json string or allocated
	struct _Leaf	*elements; // array and hash elements
	VALUE		value;
    };
    uint8_t		rtype;
    uint8_t		parent_type;
    uint8_t		value_type;
} *Leaf;

extern VALUE	oj_saj_parse(int argc, VALUE *argv, VALUE self);
extern VALUE	oj_sc_parse(int argc, VALUE *argv, VALUE self);

extern VALUE	oj_strict_parse(int argc, VALUE *argv, VALUE self);
extern VALUE	oj_strict_sparse(int argc, VALUE *argv, VALUE self);
extern VALUE	oj_compat_parse(int argc, VALUE *argv, VALUE self);
extern VALUE	oj_object_parse(int argc, VALUE *argv, VALUE self);

extern VALUE	oj_strict_parse_cstr(int argc, VALUE *argv, char *json, size_t len);
extern VALUE	oj_compat_parse_cstr(int argc, VALUE *argv, char *json, size_t len);
extern VALUE	oj_object_parse_cstr(int argc, VALUE *argv, char *json, size_t len);

extern void	oj_parse_options(VALUE ropts, Options copts);

extern void	oj_dump_obj_to_json(VALUE obj, Options copts, Out out);
extern void	oj_dump_obj_to_json_using_params(VALUE obj, Options copts, Out out, int argc, VALUE *argv);
extern void	oj_write_obj_to_file(VALUE obj, const char *path, Options copts);
extern void	oj_write_obj_to_stream(VALUE obj, VALUE stream, Options copts);
extern void	oj_dump_leaf_to_json(Leaf leaf, Options copts, Out out);
extern void	oj_write_leaf_to_file(Leaf leaf, const char *path, Options copts);

extern void	oj_str_writer_push_key(StrWriter sw, const char *key);
extern void	oj_str_writer_push_object(StrWriter sw, const char *key);
extern void	oj_str_writer_push_array(StrWriter sw, const char *key);
extern void	oj_str_writer_push_value(StrWriter sw, VALUE val, const char *key);
extern void	oj_str_writer_push_json(StrWriter sw, const char *json, const char *key);
extern void	oj_str_writer_pop(StrWriter sw);
extern void	oj_str_writer_pop_all(StrWriter sw);

extern void	oj_init_doc(void);

extern VALUE	Oj;
extern struct _Options	oj_default_options;
#if HAS_ENCODING_SUPPORT
extern rb_encoding	*oj_utf8_encoding;
#else
extern VALUE		oj_utf8_encoding;
#endif

extern VALUE	oj_bag_class;
extern VALUE	oj_bigdecimal_class;
extern VALUE	oj_cstack_class;
extern VALUE	oj_date_class;
extern VALUE	oj_datetime_class;
extern VALUE	oj_doc_class;
extern VALUE	oj_stream_writer_class;
extern VALUE	oj_string_writer_class;
extern VALUE	oj_stringio_class;
extern VALUE	oj_struct_class;

extern VALUE	oj_slash_string;

extern ID	oj_add_value_id;
extern ID	oj_array_append_id;
extern ID	oj_array_end_id;
extern ID	oj_array_start_id;
extern ID	oj_as_json_id;
extern ID	oj_error_id;
extern ID	oj_file_id;
extern ID	oj_fileno_id;
extern ID	oj_ftype_id;
extern ID	oj_hash_end_id;
extern ID	oj_hash_key_id;
extern ID	oj_hash_set_id;
extern ID	oj_hash_start_id;
extern ID	oj_iconv_id;
extern ID	oj_instance_variables_id;
extern ID	oj_json_create_id;
extern ID	oj_length_id;
extern ID	oj_new_id;
extern ID	oj_parse_id;
extern ID	oj_pos_id;
extern ID	oj_read_id;
extern ID	oj_readpartial_id;
extern ID	oj_replace_id;
extern ID	oj_stat_id;
extern ID	oj_string_id;
extern ID	oj_to_hash_id;
extern ID	oj_to_json_id;
extern ID	oj_to_s_id;
extern ID	oj_to_sym_id;
extern ID	oj_to_time_id;
extern ID	oj_tv_nsec_id;
extern ID	oj_tv_sec_id;
extern ID	oj_tv_usec_id;
extern ID	oj_utc_id;
extern ID	oj_utc_offset_id;
extern ID	oj_utcq_id;
extern ID	oj_write_id;

#if USE_PTHREAD_MUTEX
extern pthread_mutex_t	oj_cache_mutex;
#elif USE_RB_MUTEX
extern VALUE	oj_cache_mutex;
#endif

#if defined(__cplusplus)
#if 0
{ /* satisfy cc-mode */
#endif
}  /* extern "C" { */
#endif
#endif /* __OJ_H__ */
