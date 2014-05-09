/* reader.c
 * Copyright (c) 2011, Peter Ohler
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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#if NEEDS_UIO
#include <sys/uio.h>	
#endif
#include <unistd.h>
#include <time.h>

#include "ruby.h"
#include "oj.h"
#include "reader.h"

#define BUF_PAD	4

static VALUE		rescue_cb(VALUE rdr, VALUE err);
static VALUE		io_cb(VALUE rdr);
static VALUE		partial_io_cb(VALUE rdr);
static int		read_from_io(Reader reader);
#ifndef JRUBY_RUBY
static int		read_from_fd(Reader reader);
#endif
static int		read_from_io_partial(Reader reader);
static int		read_from_str(Reader reader);

void
oj_reader_init(Reader reader, VALUE io) {
    if (oj_stringio_class == rb_obj_class(io)) {
	VALUE	s = rb_funcall2(io, oj_string_id, 0, 0);

	reader->read_func = read_from_str;
	reader->in_str = StringValuePtr(s);
    } else if (rb_respond_to(io, oj_readpartial_id)) {
	VALUE	rfd;

	if (rb_respond_to(io, oj_fileno_id) && Qnil != (rfd = rb_funcall(io, oj_fileno_id, 0))) {
	    reader->read_func = read_from_fd;
	    reader->fd = FIX2INT(rfd);
	} else {
	    reader->read_func = read_from_io_partial;
	    reader->io = io;
	}
    } else if (rb_respond_to(io, oj_read_id)) {
	VALUE	rfd;

	if (rb_respond_to(io, oj_fileno_id) && Qnil != (rfd = rb_funcall(io, oj_fileno_id, 0))) {
	    reader->read_func = read_from_fd;
	    reader->fd = FIX2INT(rfd);
	} else {
	    reader->read_func = read_from_io;
	    reader->io = io;
	}
    } else {
	// TBD
	rb_raise(rb_eException, "parser io argument must respond to readpartial() or read().\n");
    }
    reader->head = reader->base;
    *reader->head = '\0';
    reader->end = reader->head + sizeof(reader->base) - BUF_PAD;
    reader->tail = reader->head;
    reader->read_end = reader->head;
    reader->pro = 0;
    reader->str = 0;
    reader->line = 1;
    reader->col = 0;
    reader->pro_line = 1;
    reader->pro_col = 0;
}

int
oj_reader_read(Reader reader) {
    int		err;
    size_t	shift = 0;
    
    // if there is not much room to read into, shift or realloc a larger buffer.
    if (reader->head < reader->tail && 4096 > reader->end - reader->tail) {
	if (0 == reader->pro) {
	    shift = reader->tail - reader->head;
	} else {
	    shift = reader->pro - reader->head - 1; // leave one character so we cab backup one
	}
	if (0 >= shift) { /* no space left so allocate more */
	    char	*old = reader->head;
	    size_t	size = reader->end - reader->head + BUF_PAD;
	
	    if (reader->head == reader->base) {
		reader->head = ALLOC_N(char, size * 2);
		memcpy(reader->head, old, size);
	    } else {
		REALLOC_N(reader->head, char, size * 2);
	    }
	    reader->end = reader->head + size * 2 - BUF_PAD;
	    reader->tail = reader->head + (reader->tail - old);
	    reader->read_end = reader->head + (reader->read_end - old);
	    if (0 != reader->pro) {
		reader->pro = reader->head + (reader->pro - old);
	    }
	    if (0 != reader->str) {
		reader->str = reader->head + (reader->str - old);
	    }
	} else {
	    memmove(reader->head, reader->head + shift, reader->read_end - (reader->head + shift));
	    reader->tail -= shift;
	    reader->read_end -= shift;
	    if (0 != reader->pro) {
		reader->pro -= shift;
	    }
	    if (0 != reader->str) {
		reader->str -= shift;
	    }
	}
    }
    err = reader->read_func(reader);
    *reader->read_end = '\0';

    return err;
}

static VALUE
rescue_cb(VALUE rbuf, VALUE err) {
#if (defined(RUBINIUS_RUBY) || (1 == RUBY_VERSION_MAJOR && 8 == RUBY_VERSION_MINOR))
    if (rb_obj_class(err) != rb_eTypeError) {
#else
    if (rb_obj_class(err) != rb_eEOFError) {
#endif
	Reader	reader = (Reader)rbuf;

	//oj_sax_drive_cleanup(reader->dr); called after exiting protect
	rb_raise(err, "at line %d, column %d\n", reader->line, reader->col);
    }
    return Qfalse;
}

static VALUE
partial_io_cb(VALUE rbuf) {
    Reader	reader = (Reader)rbuf;
    VALUE	args[1];
    VALUE	rstr;
    char	*str;
    size_t	cnt;

    args[0] = ULONG2NUM(reader->end - reader->tail);
    rstr = rb_funcall2(reader->io, oj_readpartial_id, 1, args);
    str = StringValuePtr(rstr);
    cnt = strlen(str);
    //printf("*** read %lu bytes, str: '%s'\n", cnt, str);
    strcpy(reader->tail, str);
    reader->read_end = reader->tail + cnt;

    return Qtrue;
}

static VALUE
io_cb(VALUE rbuf) {
    Reader	reader = (Reader)rbuf;
    VALUE	args[1];
    VALUE	rstr;
    char	*str;
    size_t	cnt;

    args[0] = ULONG2NUM(reader->end - reader->tail);
    rstr = rb_funcall2(reader->io, oj_read_id, 1, args);
    str = StringValuePtr(rstr);
    cnt = strlen(str);
    /*printf("*** read %lu bytes, str: '%s'\n", cnt, str); */
    strcpy(reader->tail, str);
    reader->read_end = reader->tail + cnt;

    return Qtrue;
}

static int
read_from_io_partial(Reader reader) {
    return (Qfalse == rb_rescue(partial_io_cb, (VALUE)reader, rescue_cb, (VALUE)reader));
}

static int
read_from_io(Reader reader) {
    return (Qfalse == rb_rescue(io_cb, (VALUE)reader, rescue_cb, (VALUE)reader));
}

static int
read_from_fd(Reader reader) {
    ssize_t	cnt;
    size_t	max = reader->end - reader->tail;

    cnt = read(reader->fd, reader->tail, max);
    if (cnt < 0) {
	// TBD	      oj_sax_drive_error(reader->dr, "failed to read from file");
	return -1;
    } else if (0 != cnt) {
	reader->read_end = reader->tail + cnt;
    }
    return 0;
}

static char*
oj_stpncpy(char *dest, const char *src, size_t n) {
    size_t	cnt = strlen(src) + 1;

    if (n < cnt) {
	cnt = n;
    }
    strncpy(dest, src, cnt);

    return dest + cnt - 1;
}


static int
read_from_str(Reader reader) {
    size_t	max = reader->end - reader->tail - 1;
    char	*s;
    long	cnt;

    if ('\0' == *reader->in_str) {
	/* done */
	return -1;
    }
    s = oj_stpncpy(reader->tail, reader->in_str, max);
    *s = '\0';
    cnt = s - reader->tail;
    reader->in_str += cnt;
    reader->read_end = reader->tail + cnt;

    return 0;
}
