/* parse.c
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
#include <string.h>

#include "ruby.h"
#include "oj.h"

//static void	read_instruction(PInfo pi);
//static void	read_doctype(PInfo pi);
//static void	read_comment(PInfo pi);
//static void	read_element(PInfo pi);
//static void	read_text(PInfo pi);
//static void	read_cdata(PInfo pi);
//static char*	read_name_token(PInfo pi);
//static char*	read_quoted_value(PInfo pi);
//static int	read_coded_char(PInfo pi);
//static void	next_non_white(PInfo pi);
//static int	collapse_special(char *str);

static void	read_next(PInfo pi);
static void	read_obj(PInfo pi);
static void	read_array(PInfo pi);
static void	read_str(PInfo pi);
static void	read_num(PInfo pi);
static void	read_true(PInfo pi);
static void	read_false(PInfo pi);
static void	read_nil(PInfo pi);


/* This XML parser is a single pass, destructive, callback parser. It is a
 * single pass parse since it only make one pass over the characters in the
 * XML document string. It is destructive because it re-uses the content of
 * the string for values in the callback and places \0 characters at various
 * places to mark the end of tokens and strings. It is a callback parser like
 * a SAX parser because it uses callback when document elements are
 * encountered.
 *
 * Parsing is very tolerant. Lack of headers and even mispelled element
 * endings are passed over without raising an error. A best attempt is made in
 * all cases to parse the string.
 */

inline static void
next_non_white(PInfo pi) {
    for (; 1; pi->s++) {
	switch(*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	    break;
	default:
	    return;
	}
    }
}

inline static void
next_white(PInfo pi) {
    for (; 1; pi->s++) {
	switch(*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	case '\0':
	    return;
	default:
	    break;
	}
    }
}

VALUE
parse(char *json, ParseCallbacks pcb, char **endp, int trace) {
    struct _PInfo	pi;

    if (0 == json) {
	raise_error("Invalid arg, xml string can not be null", json, 0);
    }
    if (trace) {
	printf("Parsing JSON:\n%s\n", json);
    }
    /* initialize parse info */
    pi.str = json;
    pi.s = json;
    pi.h = 0;
    pi.pcb = pcb;
    pi.obj = Qnil;
    pi.encoding = 0;
    pi.trace = trace;
    read_next(&pi);
    next_non_white(&pi);	// skip white space
    if ('\0' != *pi.s) {
	raise_error("invalid format, extra characters", pi.str, pi.s);
    }
    return pi.obj;
}

static void
read_next(PInfo pi) {
    next_non_white(pi);	// skip white space
    switch (*pi->s) {
    case '{':
	pi->s++;
	read_obj(pi);
	break;
    case '[':
	pi->s++;
	read_array(pi);
	break;
    case '"':
	pi->s++;
	read_str(pi);
	break;
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	read_num(pi);
    break;
    case 't':
	read_true(pi);
	break;
    case 'f':
	read_false(pi);
	break;
    case 'n':
	read_nil(pi);
	break;
    case '\0':
	break;
    default:
	break;
    }
}

static void
read_obj(PInfo pi) {
}

static void
read_array(PInfo pi) {
    if (0 != pi->pcb->add_array) {
	pi->pcb->add_array(pi);
    }
    while (1) {
	read_next(pi);
	next_non_white(pi);	// skip white space
	if (',' == *pi->s) {
	    pi->s++;
	} else if (']' == *pi->s) {
	    pi->s++;
	    break;
	} else {
	    raise_error("invalid format, expected , or ] while in an array", pi->str, pi->s);
	}
    }
    if (0 != pi->pcb->end_array) {
	pi->pcb->end_array(pi);
    }
}

static void
read_str(PInfo pi) {
}

static void
read_num(PInfo pi) {
}

static void
read_true(PInfo pi) {
    pi->s++;
    if ('r' != *pi->s || 'u' != *(pi->s + 1) || 'e' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'true'", pi->str, pi->s);
    }
    pi->s += 3;
    if (0 != pi->pcb->add_true) {
	pi->pcb->add_true(pi);
    }
}

static void
read_false(PInfo pi) {
    pi->s++;
    if ('a' != *pi->s || 'l' != *(pi->s + 1) || 's' != *(pi->s + 2) || 'e' != *(pi->s + 3)) {
	raise_error("invalid format, expected 'false'", pi->str, pi->s);
    }
    pi->s += 4;
    if (0 != pi->pcb->add_false) {
	pi->pcb->add_false(pi);
    }
}

static void
read_nil(PInfo pi) {
    pi->s++;
    if ('u' != *pi->s || 'l' != *(pi->s + 1) || 'l' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'nil'", pi->str, pi->s);
    }
    pi->s += 3;
    if (0 != pi->pcb->add_nil) {
	pi->pcb->add_nil(pi);
    }
}


#if 0
/* Entered after the "<?" sequence. Ready to read the rest.
 */
static void
read_instruction(PInfo pi) {
    struct _Attr	attrs[MAX_ATTRS + 1];
    Attr		a = attrs;
    char		*target;
    char		*end;
    char		c;
	
    memset(attrs, 0, sizeof(attrs));
    target = read_name_token(pi);
    end = pi->s;
    next_non_white(pi);
    c = *pi->s;
    *end = '\0'; // terminate name
    if ('?' != c) {
	while ('?' != *pi->s) {
	    if ('\0' == *pi->s) {
		raise_error("invalid format, processing instruction not terminated", pi->str, pi->s);
	    }
	    next_non_white(pi);
	    a->name = read_name_token(pi);
	    end = pi->s;
	    next_non_white(pi);
	    if ('=' != *pi->s++) {
		raise_error("invalid format, no attribute value", pi->str, pi->s);
	    }
	    *end = '\0'; // terminate name
	    // read value
	    next_non_white(pi);
	    a->value = read_quoted_value(pi);
	    a++;
	    if (MAX_ATTRS <= (a - attrs)) {
		raise_error("too many attributes", pi->str, pi->s);
	    }
	}
	if ('?' == *pi->s) {
	    pi->s++;
	}
    } else {
	pi->s++;
    }
    if ('>' != *pi->s++) {
	raise_error("invalid format, processing instruction not terminated", pi->str, pi->s);
    }
    if (0 != pi->pcb->instruct) {
	pi->pcb->instruct(pi, target, attrs);
    }
}

/* Entered after the "<!DOCTYPE" sequence plus the first character after
 * that. Ready to read the rest. Returns error code.
 */
static void
read_doctype(PInfo pi) {
    char	*docType;
    int		depth = 1;
    char	c;

    next_non_white(pi);
    docType = pi->s;
    while (1) {
	c = *pi->s++;
	if ('\0' == c) {
	    raise_error("invalid format, prolog not terminated", pi->str, pi->s);
	} else if ('<' == c) {
	    depth++;
	} else if ('>' == c) {
	    depth--;
	    if (0 == depth) {	/* done, at the end */
		pi->s--;
		break;
	    }
	}
    }
    *pi->s = '\0';
    pi->s++;
    if (0 != pi->pcb->add_doctype) {
	pi->pcb->add_doctype(pi, docType);
    }
}

/* Entered after "<!--". Returns error code.
 */
static void
read_comment(PInfo pi) {
    char	*end;
    char	*s;
    char	*comment;
    int		done = 0;
    
    next_non_white(pi);
    comment = pi->s;
    end = strstr(pi->s, "-->");
    if (0 == end) {
	raise_error("invalid format, comment not terminated", pi->str, pi->s);
    }
    for (s = end - 1; pi->s < s && !done; s--) {
	switch(*s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	    break;
	default:
	    *(s + 1) = '\0';
	    done = 1;
	    break;
	}
    }
    *end = '\0'; // in case the comment was blank
    pi->s = end + 3;
    if (0 != pi->pcb->add_comment) {
	pi->pcb->add_comment(pi, comment);
    }
}

/* Entered after the '<' and the first character after that. Returns status
 * code.
 */
static void
read_element(PInfo pi) {
    struct _Attr	attrs[MAX_ATTRS];
    Attr		ap = attrs;
    char		*name;
    char		*ename;
    char		*end;
    char		c;
    long		elen;
    int			hasChildren = 0;
    int			done = 0;

    ename = read_name_token(pi);
    end = pi->s;
    elen = end - ename;
    next_non_white(pi);
    c = *pi->s;
    *end = '\0';
    if ('/' == c) {
	/* empty element, no attributes and no children */
	pi->s++;
	if ('>' != *pi->s) {
	    //printf("*** '%s' ***\n", pi->s);
	    raise_error("invalid format, element not closed", pi->str, pi->s);
	}
	pi->s++;	/* past > */
	ap->name = 0;
	pi->pcb->add_element(pi, ename, attrs, hasChildren);
	pi->pcb->end_element(pi, ename);

	return;
    }
    /* read attribute names until the close (/ or >) is reached */
    while (!done) {
	if ('\0' == c) {
	    next_non_white(pi);
	    c = *pi->s;
	}
	switch (c) {
	case '\0':
	    raise_error("invalid format, document not terminated", pi->str, pi->s);
	case '/':
	    // Element with just attributes.
	    pi->s++;
	    if ('>' != *pi->s) {
		raise_error("invalid format, element not closed", pi->str, pi->s);
	    }
	    pi->s++;
	    ap->name = 0;
	    pi->pcb->add_element(pi, ename, attrs, hasChildren);
	    pi->pcb->end_element(pi, ename);

	    return;
	case '>':
	    // has either children or a value
	    pi->s++;
	    hasChildren = 1;
	    done = 1;
	    ap->name = 0;
	    pi->pcb->add_element(pi, ename, attrs, hasChildren);
	    break;
	default:
	    // Attribute name so it's an element and the attribute will be
	    // added to it.
	    ap->name = read_name_token(pi);
	    end = pi->s;
	    next_non_white(pi);
	    if ('=' != *pi->s++) {
		raise_error("invalid format, no attribute value", pi->str, pi->s);
	    }
	    *end = '\0'; // terminate name
	    // read value
	    next_non_white(pi);
	    ap->value = read_quoted_value(pi);
	    if (0 != strchr(ap->value, '&')) {
		if (0 != collapse_special((char*)ap->value)) {
		    raise_error("invalid format, special character does not end with a semicolon", pi->str, pi->s);
		}
	    }
	    ap++;
	    if (MAX_ATTRS <= (ap - attrs)) {
		raise_error("too many attributes", pi->str, pi->s);
	    }
	    break;
	}
	c = '\0';
    }
    if (hasChildren) {
	char	*start;
	
	done = 0;
	// read children
	while (!done) {
	    start = pi->s;
	    next_non_white(pi);
	    c = *pi->s++;
	    if ('\0' == c) {
		raise_error("invalid format, document not terminated", pi->str, pi->s);
	    }
	    if ('<' == c) {
		switch (*pi->s) {
		case '!':	/* better be a comment or CDATA */
		    pi->s++;
		    if ('-' == *pi->s && '-' == *(pi->s + 1)) {
			pi->s += 2;
			read_comment(pi);
		    } else if (0 == strncmp("[CDATA[", pi->s, 7)) {
			pi->s += 7;
			read_cdata(pi);
		    } else {
			raise_error("invalid format, invalid comment or CDATA format", pi->str, pi->s);
		    }
		    break;
		case '/':
		    pi->s++;
		    name = read_name_token(pi);
		    end = pi->s;
		    next_non_white(pi);
		    c = *pi->s;
		    *end = '\0';
		    if (0 != strcmp(name, ename)) {
			raise_error("invalid format, elements overlap", pi->str, pi->s);
		    }
		    if ('>' != c) {
			raise_error("invalid format, element not closed", pi->str, pi->s);
		    }
		    pi->s++;
		    pi->pcb->end_element(pi, ename);
		    return;
		case '\0':
		    raise_error("invalid format, document not terminated", pi->str, pi->s);
		default:
		    // a child element
		    read_element(pi);
		    break;
		}
	    } else {	// read as TEXT
		pi->s = start;
		//pi->s--;
		read_text(pi);
		//read_reduced_text(pi);

		// to exit read_text with no errors the next character must be <
		if ('/' == *(pi->s + 1) &&
		    0 == strncmp(ename, pi->s + 2, elen) &&
		    '>' == *(pi->s + elen + 2)) {
		    // close tag after text so treat as a value
		    pi->s += elen + 3;
		    pi->pcb->end_element(pi, ename);
		    return;
		}
	    }
	}
    }
}

static void
read_text(PInfo pi) {
    char	buf[MAX_TEXT_LEN];
    char	*b = buf;
    char	*alloc_buf = 0;
    char	*end = b + sizeof(buf) - 2;
    char	c;
    int		done = 0;

    while (!done) {
	c = *pi->s++;
	switch(c) {
	case '<':
	    done = 1;
	    pi->s--;
	    break;
	case '\0':
	    raise_error("invalid format, document not terminated", pi->str, pi->s);
	default:
	    if ('&' == c) {
		c = read_coded_char(pi);
	    }
	    if (end <= b) {
		unsigned long	size;
		
		if (0 == alloc_buf) {
		    size = sizeof(buf) * 2;
		    if (0 == (alloc_buf = (char*)malloc(size))) {
			raise_error("text too long", pi->str, pi->s);
		    }
		    memcpy(alloc_buf, buf, b - buf);
		    b = alloc_buf + (b - buf);
		} else {
		    unsigned long	pos = b - alloc_buf;

		    size = (end - alloc_buf) * 2;
		    if (0 == (alloc_buf = (char*)realloc(alloc_buf, size))) {
			raise_error("text too long", pi->str, pi->s);
		    }
		    b = alloc_buf + pos;
		}
		end = alloc_buf + size - 2;
	    }
	    *b++ = c;
	    break;
	}
    }
    *b = '\0';
    if (0 != alloc_buf) {
	pi->pcb->add_text(pi, alloc_buf, ('/' == *(pi->s + 1)));
	free(alloc_buf);
    } else {
	pi->pcb->add_text(pi, buf, ('/' == *(pi->s + 1)));
    }
}

static char*
read_name_token(PInfo pi) {
    char	*start;

    next_non_white(pi);
    start = pi->s;
    for (; 1; pi->s++) {
	switch (*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '?':
	case '=':
	case '/':
	case '>':
	case '\n':
	case '\r':
	    return start;
	case '\0':
	    // documents never terminate after a name token
	    raise_error("invalid format, document not terminated", pi->str, pi->s);
	    break; // to avoid warnings
	default:
	    break;
	}
    }
    return start;
}

static void
read_cdata(PInfo pi) {
    char	*start;
    char	*end;

    start = pi->s;
    end = strstr(pi->s, "]]>");
    if (end == 0) {
	raise_error("invalid format, CDATA not terminated", pi->str, pi->s);
    }
    *end = '\0';
    pi->s = end + 3;
    if (0 != pi->pcb->add_cdata) {
	pi->pcb->add_cdata(pi, start, end - start);
    }
}

inline static void
next_non_token(PInfo pi) {
    for (; 1; pi->s++) {
	switch(*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	case '/':
	case '>':
	    return;
	default:
	    break;
	}
    }
}

/* Assume the value starts immediately and goes until the quote character is
 * reached again. Do not read the character after the terminating quote.
 */
static char*
read_quoted_value(PInfo pi) {
    char	*value = 0;
    
    if ('"' == *pi->s || ('\'' == *pi->s && StrictEffort != pi->effort)) {
        char	term = *pi->s;
        
        pi->s++;	// skip quote character
        value = pi->s;
        for (; *pi->s != term; pi->s++) {
            if ('\0' == *pi->s) {
                raise_error("invalid format, document not terminated", pi->str, pi->s);
            }
        }
        *pi->s = '\0'; // terminate value
        pi->s++;	   // move past quote
    } else if (StrictEffort == pi->effort) {
	raise_error("invalid format, expected a quote character", pi->str, pi->s);
    } else {
        value = pi->s;
        next_white(pi);
	if ('\0' == *pi->s) {
	    raise_error("invalid format, document not terminated", pi->str, pi->s);
        }
        *pi->s++ = '\0'; // terminate value
    }
    return value;
}

static int
read_coded_char(PInfo pi) {
    char	*b, buf[8];
    char	*end = buf + sizeof(buf);
    char	*s;
    int	c;

    for (b = buf, s = pi->s; b < end; b++, s++) {
	if (';' == *s) {
	    *b = '\0';
	    s++;
	    break;
	}
	*b = *s;
    }
    if (b > end) {
	return *pi->s;
    }
    if ('#' == *buf) {
	c = (int)strtol(buf + 1, &end, 10);
	if (0 >= c || '\0' != *end) {
	    return *pi->s;
	}
	pi->s = s;

	return c;
    }
    if (0 == strcasecmp(buf, "nbsp")) {
	pi->s = s;
	return ' ';
    } else if (0 == strcasecmp(buf, "lt")) {
	pi->s = s;
	return '<';
    } else if (0 == strcasecmp(buf, "gt")) {
	pi->s = s;
	return '>';
    } else if (0 == strcasecmp(buf, "amp")) {
	pi->s = s;
	return '&';
    } else if (0 == strcasecmp(buf, "quot")) {
	pi->s = s;
	return '"';
    } else if (0 == strcasecmp(buf, "apos")) {
	pi->s = s;
	return '\'';
    }
    return *pi->s;
}

static int
collapse_special(char *str) {
    char	*s = str;
    char	*b = str;

    while ('\0' != *s) {
	if ('&' == *s) {
	    int		c;
	    char	*end;
	    
	    s++;
	    if ('#' == *s) {
		c = (int)strtol(s, &end, 10);
		if (';' != *end) {
		    return EDOM;
		}
		s = end + 1;
	    } else if (0 == strncasecmp(s, "lt;", 3)) {
		c = '<';
		s += 3;
	    } else if (0 == strncasecmp(s, "gt;", 3)) {
		c = '>';
		s += 3;
	    } else if (0 == strncasecmp(s, "amp;", 4)) {
		c = '&';
		s += 4;
	    } else if (0 == strncasecmp(s, "quot;", 5)) {
		c = '"';
		s += 5;
	    } else if (0 == strncasecmp(s, "apos;", 5)) {
		c = '\'';
		s += 5;
	    } else {
		c = '?';
		while (';' != *s++) {
		    if ('\0' == *s) {
			return EDOM;
		    }
		}
		s++;
	    }
	    *b++ = (char)c;
	} else {
	    *b++ = *s++;
	}
    }
    *b = '\0';

    return 0;
}
#endif
