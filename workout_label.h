/*
 * decode a name or label character
 */

#ifndef WORKOUT_LABEL_H
#define WORKOUT_LABEL_H

#include "buf.h"
#include "workout_int.h"

void workout_label_extract(BUF * buf, size_t offset, S725_Label *label, int bytes);
void workout_label_encode(S725_Label label, unsigned char *buf, int bytes);

#endif /* WORKOUT_LABEL_H */
