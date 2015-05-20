/*
 * workout parsing functions
 */

#ifndef WORKOUT_H
#define WORKOUT_H

#include <stdio.h>

#include "buf.h"

#define S725_WORKOUT_HEADER   1
#define S725_WORKOUT_LAPS     2
#define S725_WORKOUT_SAMPLES  4
#define S725_WORKOUT_FULL     7

typedef struct workout_t workout_t;

workout_t*  workout_read_buf(BUF *buf);
workout_t*	workout_read(char* filename);
void 		workout_free(workout_t * w);

#endif	/* WORKOUT_H */
