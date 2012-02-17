/* oj.c
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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ruby.h"
#include "oj.h"

void Init_oj();

VALUE    Oj = Qnil;

extern ParseCallbacks   oj_gen_callbacks;


static VALUE
load(char *json, int argc, VALUE *argv, VALUE self) {
    VALUE	obj;
    
    obj = parse(json, oj_gen_callbacks, 0, 0);
    free(json);

    return obj;
}

/* call-seq: load(xml, options) => Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into a Hash, Array, String, Fixnum, Float,
 * true, false, or nil Raises an exception if the JSON is * malformed or the
 * classes specified are not valid.
 * @param [String] json JSON String
 * @param [Hash] options load options
 */
static VALUE
load_str(int argc, VALUE *argv, VALUE self) {
    char        *json;
    
    Check_Type(*argv, T_STRING);
    // the xml string gets modified so make a copy of it
    json = strdup(StringValuePtr(*argv));

    return load(json, argc - 1, argv + 1, self);
}

static VALUE
load_file(int argc, VALUE *argv, VALUE self) {
    char                *path;
    char                *json;
    FILE                *f;
    unsigned long       len;
    
    Check_Type(*argv, T_STRING);
    path = StringValuePtr(*argv);
    if (0 == (f = fopen(path, "r"))) {
        rb_raise(rb_eIOError, "%s\n", strerror(errno));
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    if (0 == (json = malloc(len + 1))) {
        fclose(f);
        rb_raise(rb_eNoMemError, "Could not allocate memory for %ld byte file.\n", len);
    }
    fseek(f, 0, SEEK_SET);
    if (len != fread(json, 1, len, f)) {
        fclose(f);
        rb_raise(rb_eLoadError, "Failed to read %ld bytes from %s.\n", len, path);
    }
    fclose(f);
    json[len] = '\0';

    return load(json, argc - 1, argv + 1, self);
}

void Init_oj() {

    Oj = rb_define_module("Oj");

    rb_define_module_function(Oj, "load", load_str, -1);
    rb_define_module_function(Oj, "load_file", load_file, -1);
}

void
_raise_error(const char *msg, const char *xml, const char *current, const char* file, int line) {
    int         xline = 1;
    int         col = 1;

    for (; xml < current && '\n' != *current; current--) {
        col++;
    }
    for (; xml < current; current--) {
        if ('\n' == *current) {
            xline++;
        }
    }
    rb_raise(rb_eSyntaxError, "%s at line %d, column %d [%s:%d]\n", msg, xline, col, file, line);
}
