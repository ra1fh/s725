/*
 * utility functions
 */

#ifndef UTILS_H
#define UTILS_H

void crc_process(unsigned short *context, unsigned char ch);
void crc_block(unsigned short *context, const unsigned char *blk, int len);

#endif	/* UTILS_H */



