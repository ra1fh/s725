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
