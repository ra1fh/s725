
#if defined(__linux__)

#include_next <string.h>
 
#ifndef COMPAT_STRING_H
#define COMPAT_STRING_H
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#else

#include_next <string.h>

#endif
