/*
 * decode a name or label character
 */

#ifndef LABEL_H
#define LABEL_H

#include "s710.h"

void label_extract(unsigned char *buf, S710_Label *label, int bytes);
void label_encode(S710_Label label, unsigned char *buf, int bytes);

#endif /* LABEL_H */
