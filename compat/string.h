
#include_next <string.h>

#ifndef COMPAT_STRING_H
#define COMPAT_STRING_H

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#endif
