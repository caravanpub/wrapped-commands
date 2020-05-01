#ifndef WRAPPERS_H
#define WRAPPERS_H
#include "rule.h"
#include "regex.h"
#include "version.h"

/* get length of pointer array */
#define PALEN(x_a) x_a == NULL ? 0 : sizeof(x_a)/sizeof(x_a[0])


#if !defined( PATH_MAX )
#   define PATH_MAX 1024
#endif

#endif
