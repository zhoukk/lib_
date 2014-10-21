#ifndef _H
#define _H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __IMPL
#include "_config.h"
#endif

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifdef __GNUC__
#define offset_of(type, field) __builtin_offsetof(type, field)
#else
#define offset_of(type, field) ((size_t)(&((type *)&0)->field))
#endif

#define container_of(ptr, type, field) ((type *)(void *)((char *)ptr - offset_of(type, field)))


typedef void *(*alloc_pt)(void *old, size_t size);

extern alloc_pt alloc;


#endif // _H
