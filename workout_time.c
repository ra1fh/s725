/* workout_time.c - time helper functions */

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

#include "workout_time.h"

/*
 * Tenths of a second.
 */
time_t
workout_time_to_tenths(S725_Time *t)
{
	time_t tm;

	tm = ((t->hours * 60 + t->minutes) * 60 + t->seconds) * 10 + t->tenths + 5;

	return tm;
}

/*
 * This probably doesn't work if the S725_Time argument is negative.
 */
void
workout_time_increment(S725_Time *t, unsigned int seconds)
{
	int hours;
	int minutes;
	int secs;

	hours   = seconds / 3600;
	minutes = (seconds / 60) % 60;
	secs    = seconds % 60;

	if (secs + t->seconds >= 60) {
		minutes++;
		secs -= 60;
	}
	if (minutes + t->minutes >= 60) {
		hours++;
		minutes -= 60;
	}

	t->seconds += secs;
	t->minutes += minutes;
	t->hours += hours;
}


void
workout_time_diff(S725_Time *t1, S725_Time *t2, S725_Time *diff)
{
	int t_t1;
	int t_t2;
	int t_diff;
	int negative = 0;

	/* first compute t1 and t2 in tenths of a second */
	t_t1 = ((t1->hours * 60 + t1->minutes) * 60 + t1->seconds) * 10 + t1->tenths;
	t_t2 = ((t2->hours * 60 + t2->minutes) * 60 + t2->seconds) * 10 + t2->tenths;

	t_diff = t_t2 - t_t1;
	if (t_diff < 0) {
		negative = 1;
		t_diff = -t_diff;
	}

	diff->tenths = t_diff % 10;
	t_diff = (t_diff - diff->tenths) / 10;

	/* now t_diff is in seconds */
	diff->seconds = t_diff % 60;
	t_diff = (t_diff - diff->seconds) / 60;

	/* now t_diff is in minutes */
	diff->minutes = t_diff % 60;
	t_diff = (t_diff - diff->minutes) / 60;

	/* the rest is the hours */
	diff->hours = t_diff;

	/* if we got a negative time, switch the sign of everything. */
	if (negative) {
		diff->hours   = -diff->hours;
		diff->minutes = -diff->minutes;
		diff->seconds = -diff->seconds;
		diff->tenths  = -diff->tenths;
	}
}

/*
 * write time to given fp
 *
 * the "format" argument can be one of:
 *
 * "hmst" => 12:34:56.7
 * "hms"  => 12:34:56
 * "hm"   => 12:34
 * "ms"   => 34:56
 * "mst"  => 34:56.7
 * "st"   => 56.7
 */
void
workout_time_print(S725_Time *t, const char *format, FILE *fp)
{
	const char *p;
	int   r = 0;

	for (p = format; *p != 0; p++) {
		switch (*p) {
		case 'h': case 'H':
			if (r) fprintf(fp, ":");
			fprintf(fp, "%d", t->hours);
			r = 1;
			break;
		case 'm': case 'M':
			if (r) fprintf(fp, ":");
			fprintf(fp, "%02d", t->minutes);
			r = 1;
			break;
		case 's': case 'S':
			if (r) fprintf(fp, ":");
			fprintf(fp, "%02d", t->seconds);
			r = 1;
			break;
		case 't': case 'T':
			if (r) fprintf(fp, ".");
			fprintf(fp, "%d", t->tenths);
			r = 1;
			break;
		default:
			break;
		}
	}
}

