// Copyright (c) 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef CACHE_H
#define CACHE_H

#include <ruby.h>

#define CACHE_MAX_KEY 35

struct _cache;

extern struct _cache *cache_create(size_t size, VALUE (*form)(const char *str, size_t len), bool reg);
extern void           cache_free(struct _cache *c);

extern VALUE cache_intern(struct _cache *c, const char *key, size_t len);

#endif /* CACHE_H */
