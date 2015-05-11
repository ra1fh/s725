/*
 * time helper functions
 */

#ifndef WORKOUT_TIME_H
#define WORKOUT_TIME_H

#include <stdio.h>

#include "workout_int.h"

time_t 		workout_time_to_tenths(S725_Time *t);
void   		workout_time_increment(S725_Time *t, unsigned int seconds);
void   		workout_time_diff(S725_Time *t1, S725_Time *t2, S725_Time *diff);
void   		workout_time_print(S725_Time* t, const char *format, FILE *fp);

#endif	/* WORKOUT_TIME_H */
