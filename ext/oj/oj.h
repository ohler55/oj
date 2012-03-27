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
#ifdef HAVE_RUBY_ENCODING_H
// HAVE_RUBY_ENCODING_H defined for Ruby 1.9
#define IVAR_HELPERS 1
#include "ruby/encoding.h"
#endif

#include "cache.h"

#ifdef JRUBY
#define NO_RSTRUCT 1
#endif

#if (defined RBX_Qnil && !defined RUBINIUS)
#define RUBINIUS
#endif

#ifdef RUBINIUS
#undef T_RATIONAL
#undef T_COMPLEX
#define NO_RSTRUCT 1
#define IVAR_HELPERS 0
#endif

#if IVAR_HELPERS
#include "ruby/st.h"
#else
#include "st.h"
#endif

#define raise_error(msg, xml, current) _oj_raise_error(msg, xml, current, __FILE__, __LINE__)

typedef enum {
    Yes    = 'y',
    No     = 'n',
    NotSet = 0
} YesNo;

typedef enum {
    StrictMode  = 's',
    ObjectMode  = 'o',
    NullMode    = 'n',
    CompatMode  = 'c'
} Mode;

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
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding	*encoding;
#else
    void	*encoding;
#endif
    //char        encoding[64];	// encoding, stored in the option to avoid GC invalidation in default values
    int         indent;		// indention for dump, default 2
    char        circular;	// YesNo
    char        auto_define;	// YesNo
    char        sym_key;	// YesNo
    char        ascii_only;	// YesNo
    char        mode;		// Mode
    DumpOpts	dump_opts;
} *Options;

enum {
    STR_VAL  = 0x00,
    COL_VAL  = 0x01,
    RUBY_VAL = 0x02
};
    
typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*key;      // hash key
	size_t		index;     // array index, 0 is not set
    };
    union {
	char		*str;      // pointer to location in json string
	struct _Leaf	*elements; // array and hash elements
	VALUE		value;
    };
    uint8_t		type;
    uint8_t		parent_type;
    uint8_t		value_type;
} *Leaf;

extern VALUE	oj_parse(char *json, Options options);
extern char*	oj_write_obj_to_str(VALUE obj, Options copts);
extern void	oj_write_obj_to_file(VALUE obj, const char *path, Options copts);
extern char*	oj_write_leaf_to_str(Leaf leaf, Options copts);
extern void	oj_write_leaf_to_file(Leaf leaf, const char *path, Options copts);

extern void	_oj_raise_error(const char *msg, const char *xml, const char *current, const char* file, int line);

extern void	oj_init_doc(void);

extern VALUE    Oj;
extern struct _Options	oj_default_options;

extern VALUE	oj_bag_class;
extern VALUE	oj_date_class;
extern VALUE	oj_doc_class;
extern VALUE	oj_stringio_class;
extern VALUE	oj_struct_class;
extern VALUE	oj_time_class;

extern VALUE	oj_slash_string;

extern ID	oj_as_json_id;
extern ID	oj_at_id;
extern ID	oj_instance_variables_id;
extern ID	oj_json_create_id;
extern ID	oj_string_id;
extern ID	oj_to_hash_id;
extern ID	oj_to_json_id;
extern ID	oj_to_sym_id;
extern ID	oj_tv_nsec_id;
extern ID	oj_tv_sec_id;
extern ID	oj_tv_usec_id;

extern Cache    oj_class_cache;
extern Cache    oj_attr_cache;

#if defined(__cplusplus)
#if 0
{ /* satisfy cc-mode */
#endif
}  /* extern "C" { */
#endif
#endif /* __OJ_H__ */
