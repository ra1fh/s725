/* workout_time.h - time helper functions */

/*
 * Copyright (C) 2016  Ralf Horstmann
 * Copyright (C) 2007  Dave Bailey  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
