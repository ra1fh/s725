/*
 * utility functions
 */

#ifndef UTILS_H
#define UTILS_H

/* helpful macros */
#define UNIB(x)      ((x)>>4)
#define LNIB(x)      ((x)&0x0f)
#define BCD(x)       (UNIB(x)*10 + LNIB(x))

#endif	/* UTILS_H */
