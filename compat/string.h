
#include_next "string.h"

#ifndef __STRING_H
#define __STRING_H

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#endif
