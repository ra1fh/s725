/*
 * workout parsing functions
 */

#ifndef WORKOUT_H
#define WORKOUT_H

#include <time.h>
#include <stdio.h>

#define S710_WORKOUT_HEADER   1
#define S710_WORKOUT_LAPS     2
#define S710_WORKOUT_SAMPLES  4
#define S710_WORKOUT_FULL     7

typedef struct S710_Time {
	int    hours;
	int    minutes;
	int    seconds;
	int    tenths;
} S710_Time;

typedef char S710_Label[8];

typedef struct workout_t workout_t;

workout_t*	workout_read(char* filename);
void		workout_print(workout_t * w, FILE * fp, int what);
int  		workout_header_size(workout_t * w);
int  		workout_bytes_per_sample(unsigned char bt);
int  		workout_allocate_sample_space(workout_t * w);
void 		workout_free(workout_t * w);

#endif	/* WORKOUT_H */
