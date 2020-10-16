// Copyright (c) 2011 Peter Ohler. All rights reserved.

#ifndef OJ_HASH_H
#define OJ_HASH_H

#include "ruby.h"

typedef struct _hash	*Hash;

extern void	oj_hash_init();

extern VALUE	oj_class_hash_get(const char *key, size_t len, VALUE **slotp);
extern ID	oj_attr_hash_get(const char *key, size_t len, ID **slotp);

extern void	oj_hash_print();
extern char*	oj_strndup(const char *s, size_t len);

#endif /* OJ_HASH_H */
