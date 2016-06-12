/* workout_print.h - workout printing functions */

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

#ifndef WORKOUT_PRINT_H
#define WORKOUT_PRINT_H

#include <stdio.h>

#include "workout.h"

#define S725_WORKOUT_HEADER   1
#define S725_WORKOUT_LAPS     2
#define S725_WORKOUT_SAMPLES  4
#define S725_WORKOUT_FULL     7

void		workout_print_txt(workout_t * w, FILE *fp, int what);
void		workout_print_hrm(workout_t *w, FILE *fp);
void		workout_print_tcx(workout_t *w, FILE *fp);

#endif	/* WORKOUT_PRINT_H */
