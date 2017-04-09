/* mimic_rails.h
 * Copyright (c) 2017, Peter Ohler
 * All rights reserved.
 */

#ifndef __OJ_MIMIC_RAILS_H__
#define __OJ_MIMIC_RAILS_H__

#include "dump.h"

extern void	oj_mimic_rails_init(VALUE oj);
extern ROpt	oj_rails_get_opt(ROptTable rot, VALUE clas);

extern bool	oj_rails_hash_opt;
extern bool	oj_rails_array_opt;

#endif /* __OJ_MIMIC_RAILS_H__ */
