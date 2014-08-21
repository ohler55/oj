/* reader.h
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

#ifndef __OJ_READER_H__
#define __OJ_READER_H__

typedef struct _Reader {
    char	base[0x00001000];
    char	*head;
    char	*end;
    char	*tail;
    char	*read_end;	/* one past last character read */
    char	*pro;		/* protection start, buffer can not slide past this point */
    char	*str;		/* start of current string being read */
    int		line;
    int		col;
    int		free_head;
    int		(*read_func)(struct _Reader *reader);
    union {
	int		fd;
	VALUE		io;
	const char	*in_str;
    };
} *Reader;

extern void	oj_reader_init(Reader reader, VALUE io, int fd);
extern int	oj_reader_read(Reader reader);

static inline char
reader_get(Reader reader) {
    //printf("*** drive get from '%s'  from start: %ld	buf: %p	 from read_end: %ld\n", reader->tail, reader->tail - reader->head, reader->head, reader->read_end - reader->tail);
    if (reader->read_end <= reader->tail) {
	if (0 != oj_reader_read(reader)) {
	    return '\0';
	}
    }
    if ('\n' == *reader->tail) {
	reader->line++;
	reader->col = 0;
    }
    reader->col++;
    
    return *reader->tail++;
}

static inline void
reader_backup(Reader reader) {
    reader->tail--;
    reader->col--;
    if (0 >= reader->col) {
	reader->line--;
	// allow col to be negative since we never backup twice in a row
    }
}

static inline void
reader_protect(Reader reader) {
    reader->pro = reader->tail;
    reader->str = reader->tail; // can't have str before pro
}

static inline void
reader_release(Reader reader) {
    reader->pro = 0;
}

/* Starts by reading a character so it is safe to use with an empty or
 * compacted buffer.
 */
static inline char
reader_next_non_white(Reader reader) {
    char	c;

    while ('\0' != (c = reader_get(reader))) {
	switch(c) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	    break;
	default:
	    return c;
	}
    }
    return '\0';
}

/* Starts by reading a character so it is safe to use with an empty or
 * compacted buffer.
 */
static inline char
reader_next_white(Reader reader) {
    char	c;

    while ('\0' != (c = reader_get(reader))) {
	switch(c) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	case '\0':
	    return c;
	default:
	    break;
	}
    }
    return '\0';
}

static inline int
reader_expect(Reader reader, const char *s) {
    for (; '\0' != *s; s++) {
	if (reader_get(reader) != *s) {
	    return -1;
	}
    }
    return 0;
}

static inline void
reader_cleanup(Reader reader) {
    if (reader->free_head && 0 != reader->head) {
	xfree((char*)reader->head);
	reader->head = 0;
	reader->free_head = 0;
    }
}

static inline int
is_white(char c) {
    switch(c) {
    case ' ':
    case '\t':
    case '\f':
    case '\n':
    case '\r':
	return 1;
    default:
	break;
    }
    return 0;
}

#endif /* __OJ_READER_H__ */
