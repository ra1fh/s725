/*
 * workout parsing functions
 */

#ifndef WORKOUT_H
#define WORKOUT_H

#include <stdio.h>

#define S710_WORKOUT_HEADER   1
#define S710_WORKOUT_LAPS     2
#define S710_WORKOUT_SAMPLES  4
#define S710_WORKOUT_FULL     7

typedef struct workout_t workout_t;

workout_t*	workout_read(char* filename);
void		workout_print(workout_t * w, FILE * fp, int what);
void 		workout_free(workout_t * w);

#endif	/* WORKOUT_H */
