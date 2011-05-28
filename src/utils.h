/*
 * utility functions
 */

#ifndef UTILS_H
#define UTILS_H

/* helpful macros */
#define UNIB(x)      ((x)>>4)
#define LNIB(x)      ((x)&0x0f)
#define BCD(x)       (UNIB(x)*10 + LNIB(x))

/* crc functions */
void crc_process(unsigned short *context, unsigned char ch);
void crc_block(unsigned short *context, const unsigned char *blk, int len);

#endif	/* UTILS_H */



