/*
 * time helper functions
 */

#ifndef TIME_HELPER_H
#define TIME_HELPER_H

#include <stdio.h>

#include "workout_int.h"

time_t 		workout_time_to_tenths(S710_Time *t);
void   		workout_time_increment(S710_Time *t, unsigned int seconds);
void   		workout_time_diff(S710_Time *t1, S710_Time *t2, S710_Time *diff);
void   		workout_time_print(S710_Time* t, const char *format, FILE *fp);

#endif	/* TIME_HELPER_H */
