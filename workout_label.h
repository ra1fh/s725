/*
 * decode a name or label character
 */

#ifndef WORKOUT_LABEL_H
#define WORKOUT_LABEL_H

#include "workout_int.h"

void workout_label_extract(unsigned char *buf, S710_Label *label, int bytes);
void workout_label_encode(S710_Label label, unsigned char *buf, int bytes);

#endif /* WORKOUT_LABEL_H */
