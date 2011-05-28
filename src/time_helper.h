/*
 * time helper functions
 */

#ifndef TIME_HELPER_H
#define TIME_HELPER_H

#include "s710.h"

time_t 		s710_time_to_tenths(S710_Time *t);
void   		s710_time_increment(S710_Time *t, unsigned int seconds);
void   		s710_time_diff(S710_Time *t1, S710_Time *t2, S710_Time *diff);
void   		s710_time_print(S710_Time* t, const char *format, FILE *fp);

#endif	/* TIME_HELPER_H */
