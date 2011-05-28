/*
 * decode a name or label character
 */

#ifndef LABEL_H
#define LABEL_H

#include "workout_int.h"

void workout_label_extract(unsigned char *buf, S710_Label *label, int bytes);
void workout_label_encode(S710_Label label, unsigned char *buf, int bytes);

#endif /* LABEL_H */
