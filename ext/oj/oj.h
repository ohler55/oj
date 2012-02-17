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

#include "ruby.h"
#ifdef HAVE_RUBY_ENCODING_H
// HAVE_RUBY_ENCODING_H defined for Ruby 1.9
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
#endif

#define raise_error(msg, xml, current) _raise_error(msg, xml, current, __FILE__, __LINE__)

#define MAX_TEXT_LEN    4096
#define MAX_DEPTH       1024

typedef enum {
    NoCode         = 0,
    ArrayCode      = 'a',
    String64Code   = 'b', // base64 encoded String
    ClassCode      = 'c',
    Symbol64Code   = 'd', // base64 encoded Symbol
    FloatCode      = 'f',
    RegexpCode     = 'g',
    HashCode       = 'h',
    FixnumCode     = 'i',
    BignumCode     = 'j',
    KeyCode        = 'k', // indicates the value is a hash key, kind of a hack
    RationalCode   = 'l',
    SymbolCode     = 'm',
    FalseClassCode = 'n',
    ObjectCode     = 'o',
    RefCode        = 'p',
    RangeCode      = 'r',
    StringCode     = 's',
    TimeCode       = 't',
    StructCode     = 'u',
    ComplexCode    = 'v',
    RawCode        = 'x',
    TrueClassCode  = 'y',
    NilClassCode   = 'z',
} Type;

typedef struct _Helper {
    ID          var;    /* Object var ID */
    VALUE       obj;    /* object created or Qundef if not appropriate */
    Type	type;
} *Helper;

typedef struct _PInfo   *PInfo;

typedef struct _ParseCallbacks {
    void        (*add_obj)(PInfo pi);
    void        (*end_obj)(PInfo pi);
    void        (*add_array)(PInfo pi);
    void        (*end_array)(PInfo pi);
    void        (*add_key)(PInfo pi, char *text);
    void        (*add_str)(PInfo pi, char *text);
    void        (*add_int)(PInfo pi, int64_t val);
    void        (*add_dub)(PInfo pi, double val);
    void        (*add_nil)(PInfo pi);
    void        (*add_true)(PInfo pi);
    void        (*add_false)(PInfo pi);
} *ParseCallbacks;

/* parse information structure */
struct _PInfo {
    struct _Helper      helpers[MAX_DEPTH];
    Helper              h;              /* current helper or 0 if not set */
    char	        *str;		/* buffer being read from */
    char	        *s;		/* current position in buffer */
    ParseCallbacks      pcb;
    VALUE		obj;
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding         *encoding;
#else
    void		*encoding;
#endif
    int                 trace;
};

extern VALUE    parse(char *json, ParseCallbacks pcb, char **endp, int trace);
extern void     _raise_error(const char *msg, const char *xml, const char *current, const char* file, int line);


extern VALUE    Oj;

#if defined(__cplusplus)
#if 0
{ /* satisfy cc-mode */
#endif
}  /* extern "C" { */
#endif
#endif /* __OJ_H__ */
