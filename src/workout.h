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

typedef enum {
	S710_HRM_AUTO    =  0,
	S710_HRM_S610    = 11,  /* same as in hrm files */
	S710_HRM_S710    = 12,  /* same as in hrm files */
	S710_HRM_S810    = 13,  /* same as in hrm files */
	S710_HRM_S625X   = 22,  /* same as in hrm files */
	S710_HRM_UNKNOWN = 255
} S710_HRM_Type;

typedef struct workout_t workout_t;

workout_t*	workout_read(char* filename, S710_HRM_Type type);
void		workout_print(workout_t * w, FILE * fp, int what);
int  		workout_header_size(workout_t * w);
int  		workout_bytes_per_lap(S710_HRM_Type type, unsigned char bt, unsigned char bi);
int  		workout_bytes_per_sample(unsigned char bt);
int  		workout_allocate_sample_space(workout_t * w);
void 		workout_free(workout_t * w);

#endif	/* WORKOUT_H */
