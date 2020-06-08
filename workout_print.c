/* workout_print.c - workout printing functions */

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

#include <string.h>

#include "log.h"
#include "workout_print.h"
#include "workout_int.h"
#include "workout_time.h"

/*
 * Print workout in non-standard plain text format for
 * easy processing with gnuplot
 *
 * "what" is the bitwise or of at least one of:
 * S725_WORKOUT_HEADER
 * S725_WORKOUT_LAPS
 * S725_WORKOUT_SAMPLES
 * or it can just be S725_WORKOUT_FULL (everything)
 */
void
workout_print_txt(workout_t *w, FILE *fp, int what)
{
	const char* hrm_type = "Unknown";
	int i;
	int j;
	float vam;
	char buf[BUFSIZ];
	lap_data_t *l;
	S725_Time s;

	if (what & S725_WORKOUT_HEADER) {
		/* exercise date */
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S (%a, %d %b %Y)",
				 localtime(&w->unixtime));
		fprintf(fp, "# Workout date:          %s\n", buf);

		/* HRM type */
		switch (w->type) {
		case S725_HRM_S610:
			hrm_type = "S610";
			break;
		case S725_HRM_S625:
			hrm_type = "S625";
			break;
		case S725_HRM_S725:
			hrm_type = "S725";
			break;
		default:
			break;
		}
		fprintf(fp, "# HRM Type:              %s\n", hrm_type);

		/* user id */
		fprintf(fp, "# User ID:               %d\n", w->user_id);

		/* exercise number and label */
		if (w->exercise_number > 0 && w->exercise_number <= 5) {
			fprintf(fp, "# Exercise:              %d (%s)\n",
					w->exercise_number,
					w->exercise_label);
		}

		/* workout mode */
		fprintf(fp, "# Mode:                  HR");
		if (S725_HAS_ALTITUDE(w->mode)) fprintf(fp, ", Altitude");
		if (S725_HAS_SPEED(w->mode)) {
			fprintf(fp, ", Speed ");
			if (S725_HAS_SPEED1(w->mode) && S725_HAS_SPEED2(w->mode))
				fprintf(fp, "(Bike 2)");
			else if (S725_HAS_SPEED1(w->mode))
				fprintf(fp, "(Run)");
			else
				fprintf(fp, "(Bike 1)");
			fprintf(fp, "%s%s",
					S725_HAS_POWER(w->mode) ? ", Power" : "",
					S725_HAS_CADENCE(w->mode) ? ", Cadence" : "");
		}
		fprintf(fp, "\n");

		/* exercise duration */
		fprintf(fp, "# Exercise duration:     ");
		workout_time_print(&w->duration, "hmst", fp);
		fprintf(fp, "\n");

		if (S725_HAS_SPEED(w->mode))
			fprintf(fp, "# Exercise distance:     %.1f %s\n",
					w->exercise_distance/10.0, w->units.distance);

		/* recording interval */
		fprintf(fp, "# Recording interval:    %d seconds\n",
				w->recording_interval);

		/* average, maximum heart rate */
		fprintf(fp, "# Average heart rate:    %d bpm\n", w->avg_hr);
		fprintf(fp, "# Maximum heart rate:    %d bpm\n", w->max_hr);

		/* average, maximum cadence */
		if (S725_HAS_CADENCE(w->mode)) {
			fprintf(fp, "# Average cadence:       %d rpm\n", w->avg_cad);
			fprintf(fp, "# Maximum cadence:       %d rpm\n", w->max_cad);
		}

		/* average, maximum speed */
		if (S725_HAS_SPEED(w->mode)) {
			fprintf(fp, "# Average speed:         %.1f %s\n",
					w->avg_speed/16.0, w->units.speed);
			fprintf(fp, "# Maximum speed:         %.1f %s\n",
					w->max_speed/16.0, w->units.speed);
		}

		if (w->type != S725_HRM_S610) {
			/* min, avg, max temperature */
			fprintf(fp, "# Minimum temperature:   %d %s\n",
					w->min_temp, w->units.temperature);
			fprintf(fp, "# Average temperature:   %d %s\n",
					w->avg_temp, w->units.temperature);
			fprintf(fp, "# Maximum temperature:   %d %s\n",
					w->max_temp, w->units.temperature);
		}

		/* altitude, ascent */
		if (S725_HAS_ALTITUDE(w->mode)) {
			fprintf(fp, "# Minimum altitude:      %d %s\n",
					w->min_alt, w->units.altitude);
			fprintf(fp, "# Average altitude:      %d %s\n",
					w->avg_alt, w->units.altitude);
			fprintf(fp, "# Maximum altitude:      %d %s\n",
					w->max_alt, w->units.altitude);
			fprintf(fp, "# Ascent:                %d %s\n",
					w->ascent, w->units.altitude);
		}

		/* power data */
		if (S725_HAS_POWER(w->mode)) {
			fprintf(fp, "# Average power:         %d W\n",
					w->avg_power.power);
			fprintf(fp, "# Average LR balance:    %d-%d\n",
					w->avg_power.lr_balance >> 1,
					100 - (w->avg_power.lr_balance >> 1));
			fprintf(fp, "# Average pedal index:   %d %%\n",
					w->avg_power.pedal_index >> 1);
			fprintf(fp, "# Maximum power:         %d W\n",
					w->max_power.power);
			fprintf(fp, "# Maximum pedal index:   %d %%\n",
					w->max_power.pedal_index >> 1);
		}

		/* energy, total energy (units??) */
		fprintf(fp, "# Energy:                %d\n", w->energy);
		fprintf(fp, "# Total energy:          %d\n", w->total_energy);

		/* cumulative counters */
		fprintf(fp, "# Cumulative exercise:   ");
		workout_time_print(&w->cumulative_exercise, "hm", fp);
		fprintf(fp, "\n");

		if (S725_HAS_SPEED(w->mode)) {
			fprintf(fp, "# Cumulative ride time:  ");
			workout_time_print(&w->cumulative_ride,"hm", fp);
			fprintf(fp, "\n");
			fprintf(fp, "# Odometer:              %d %s\n",
					w->odometer, w->units.distance);
		}
		/* laps */
		fprintf(fp, "# Laps:                  %d\n", w->laps);

		fprintf(fp, "#\n");

		/* HR limits */
		for (i = 0; i < 3; i++) {
			fprintf(fp, "#\n");
			fprintf(fp, "# HR Limit %d:            %d to %3d\n",
					i+1,w->hr_limit[i].lower,w->hr_limit[i].upper);
			fprintf(fp, "#   Time below:          ");
			workout_time_print(&w->hr_zone[i][0],"hms", fp);
			fprintf(fp, "\n");
			fprintf(fp, "#   Time within:         ");
			workout_time_print(&w->hr_zone[i][1],"hms", fp);
			fprintf(fp, "\n");
			fprintf(fp, "#   Time above:          ");
			workout_time_print(&w->hr_zone[i][2],"hms", fp);
			fprintf(fp, "\n");
		}

	}
	fprintf(fp, "#\n#\n");

	if (what & S725_WORKOUT_LAPS) {
		/* lap data */
		for (i = 0; i < w->laps; i++) {
			l = &w->lap_data[i];
			fprintf(fp, "# Lap %d:\n", i+1);
			fprintf(fp, "#   Lap split:           ");
			workout_time_print(&l->split, "hmst", fp);
			fprintf(fp, "\n");
			fprintf(fp, "#   Lap cumulative:      ");
			workout_time_print(&l->cumulative, "hmst", fp);
			fprintf(fp, "\n");
			fprintf(fp, "#   Lap HR:              %d bpm\n", l->lap_hr);
			fprintf(fp, "#   Average HR:          %d bpm\n", l->avg_hr);
			fprintf(fp, "#   Maximum HR:          %d bpm\n", l->max_hr);

			if (S725_HAS_ALTITUDE(w->mode)) {
				fprintf(fp, "#   Lap altitude:        %d %s\n",
						l->alt, w->units.altitude);
				fprintf(fp, "#   Lap ascent:          %d %s\n",
						l->ascent, w->units.altitude);
				fprintf(fp, "#   Lap cumulat. asc:    %d %s\n",
						l->cumul_ascent, w->units.altitude);
				fprintf(fp, "#   Lap temperature:     %d %s\n",
						l->temp, w->units.temperature);
			}

			if (S725_HAS_CADENCE(w->mode))
				fprintf(fp, "#   Lap cadence:         %d rpm\n", l->cad);

			if (S725_HAS_SPEED(w->mode)) {
				float  lap_speed = 0;
				time_t tenths;

				fprintf(fp, "#   Lap distance:        %.1f %s\n",
						l->distance/10.0, w->units.distance);
				fprintf(fp, "#   Lap cumulat. dist:   %.1f %s\n",
						l->cumul_distance/10.0, w->units.distance);
				fprintf(fp, "#   Lap speed at end:    %.1f %s\n",
						l->speed/16.0, w->units.speed);
				tenths = workout_time_to_tenths(&l->split);
				if (tenths > 0) {
					/* note the cancelling factors of 10 */
					lap_speed = l->distance / ((float)tenths/3600.0);
				}
				fprintf(fp, "#   Lap speed ave:       %.1f %s\n",
						lap_speed, w->units.speed);
			}

			if (S725_HAS_POWER(w->mode)) {
				fprintf(fp, "#   Lap power:           %d W\n",
						l->power.power);
				fprintf(fp, "#   Lap LR balance:      %d-%d\n",
						l->power.lr_balance >> 1,
						100 - (l->power.lr_balance >> 1));
				fprintf(fp, "#   Lap pedal index:     %d%%\n",
						l->power.pedal_index >> 1);
			}

			fprintf(fp, "#\n");
		}
	}

	if (what & S725_WORKOUT_SAMPLES) {
		/* sample data */
		fprintf(fp, "#\n# Recorded data:\n#\n");
		fprintf(fp, "#   Time\t HR");

		if (S725_HAS_ALTITUDE(w->mode)) {
			fprintf(fp, "\t Alt\t    VAM");
		}

		if (S725_HAS_SPEED(w->mode)) {
			fprintf(fp, "\t  Spd");
			fprintf(fp, "\t  Dist");
			if (S725_HAS_POWER(w->mode)) {
				fprintf(fp, "\tPower");
				fprintf(fp, "\t   LR");
				fprintf(fp, "\tPI");
			}
			if (S725_HAS_CADENCE(w->mode)) {
				fprintf(fp, "\tCad");
			}
		}
		fprintf(fp, "\n");

		memset(&s, 0, sizeof(s));
		for (i = 0; i < w->samples; i++) {
			workout_time_print(&s, "hms", fp);
			fprintf(fp, "\t%3d", w->hr_data[i]);

			if (S725_HAS_ALTITUDE(w->mode)) {
				/* compute VAM as the average of the past 60 seconds... */
				vam = 0.0;
				if (w->recording_interval != 0.0) {
					j = (i >= 60/w->recording_interval) ? i-60/w->recording_interval : 0;
					if (i > j) {
						vam = (float)(w->alt_data[i] - w->alt_data[j]) * 3600.0 /
							((i-j) * w->recording_interval);
					}
				}
				fprintf(fp, "\t%4d\t%7.1f", w->alt_data[i], vam);
			}

			if (S725_HAS_SPEED(w->mode)) {
				fprintf(fp, "\t%5.1f", w->speed_data[i]/16.0);
				fprintf(fp, "\t%6.2f", w->dist_data[i]);
				if (S725_HAS_POWER(w->mode)) {
					fprintf(fp, "\t%5d\t%2d-%2d\t%2d",
							w->power_data[i].power,
							w->power_data[i].lr_balance >> 1,
							100 - (w->power_data[i].lr_balance >> 1),
							w->power_data[i].pedal_index >> 1);
				}

				if (S725_HAS_CADENCE(w->mode)) {
					fprintf(fp, "\t%3d", w->cad_data[i]);
				}
			}
			fprintf(fp, "\n");

			workout_time_increment(&s, w->recording_interval);
		}
	}
}

/*
 * This function takes a workout_t *w and dumps it to FILE *fp in HRM
 * format.
 *
 * For a description of the HRM file format, see:
 * http://developer.polar.fi/developer.nsf/main
 * The HRM file format version produced is version 1.06.
 *
 * Note: According to the HRM file format spec, we need CR/LF ("\r\n")
 * to terminate lines.  So that's why there are all those \r's floating
 * around in there, in case you were wondering.
 */
void
workout_print_hrm(workout_t *w, FILE *fp)
{
	int i, j;
	unsigned long sum_altitude;
	unsigned long sum_cadence;
	unsigned long sum_speed;
	unsigned long sum_power;
	int above_max;
	int upper_to_max;
	int lower_to_upper;
	int rest_to_lower;
	int below_rest;
	int sampled_seconds;
	int lap_start_sample;
	int lap_end_sample;
	int lap_min_hr;
	int mode;

	/* sanity checks. */
	if (w == NULL || fp == NULL) {
		log_error("workout_print_tcx: improper usage(%p,%p)", w, fp);
		return;
	}

	if (w->type == S725_HRM_UNKNOWN) {
		log_error("workout_print_tcx: unknown HRM model");
		return;
	}

	/*
	 * Sometimes users leave the watch in cycling mode when they don't
	 * mean to.  Either they forget to change modes, or they just don't
	 * feel like it.  The Polar software seems to "correct" for this by
	 * detecting if the user actually recorded any meaningful speed,
	 * power and/or cadence data.  If they didn't, then the corresponding
	 * flag is turned off, regardless of whether it was actually set in
	 * the watch.  Seems like a reasonable idea, so let's do the same.
	*/

	sum_altitude = 0;
	sum_cadence  = 0;
	sum_speed    = 0;
	sum_power    = 0;
	mode         = w->mode;

	if ( w->alt_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_altitude += w->alt_data[i];
	if ( w->cad_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_cadence  += w->cad_data[i];
	if ( w->speed_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_speed    += w->speed_data[i];
	if ( w->power_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_power    += w->power_data[i].power;

	if ( sum_altitude == 0 )  mode &= ~S725_MODE_ALTITUDE;
	if ( sum_cadence  == 0 )  mode &= ~S725_MODE_CADENCE;
	if ( sum_speed    == 0 )  mode &= ~S725_MODE_SPEED;
	if ( sum_power    == 0 )  mode &= ~S725_MODE_POWER;

	/* Enough goofing off, let's get on with it. */

	/* Print version and monitor numbers */

	fprintf(fp,"[Params]\r\nVersion=106\r\nMonitor=%d\r\n",w->type);

	/* Print the SMode flags (to our knowledge) */

	fprintf(fp,"SMode=%d%d%d%d%d%d%d%d0\r\n",
			( S725_HAS_SPEED(mode) )    ? 1 : 0,
			( S725_HAS_CADENCE(mode) )  ? 1 : 0,
			( S725_HAS_ALTITUDE(mode) ) ? 1 : 0,
			( S725_HAS_POWER(mode) )    ? 1 : 0,
			( S725_HAS_POWER(mode) )    ? 1 : 0, /* FIX ME SOMEDAY */
			( S725_HAS_POWER(mode) )    ? 1 : 0, /* FIX ME SOMEDAY */
			( S725_HAS_SPEED(mode) )    ? 1 : 0,
			(w->units.system == S725_UNITS_ENGLISH) ? 1 : 0);

	/* Date, StartTime, Length, Interval */

	fprintf(fp,"Date=%4d%02d%02d\r\n",
			w->date.tm_year + 1900,
			w->date.tm_mon + 1,
			w->date.tm_mday);

	fprintf(fp,"StartTime=%02d:%02d:%02d.0\r\n",
			w->date.tm_hour,
			w->date.tm_min,
			w->date.tm_sec);

	fprintf(fp,"Length=%02d:%02d:%02d.%d\r\n",
			w->duration.hours,
			w->duration.minutes,
			w->duration.seconds,
			w->duration.tenths);

	fprintf(fp,"Interval=%d\r\n",w->recording_interval);

	/* Limits 1, 2 and 3 */

	fprintf(fp,
			"Upper1=%d\r\nLower1=%d\r\n"
			"Upper2=%d\r\nLower2=%d\r\n"
			"Upper3=%d\r\nLower3=%d\r\n",
			w->hr_limit[0].upper,
			w->hr_limit[0].lower,
			w->hr_limit[1].upper,
			w->hr_limit[1].lower,
			w->hr_limit[2].upper,
			w->hr_limit[2].lower);

	/* Exercise timers 1, 2 and 3 */

	fprintf(fp,
			"Timer1=00:00:00.0\r\n"
			"Timer2=00:00:00.0\r\n"
			"Timer3=00:00:00.0\r\n");

	/* ActiveLimit - I think this is set using the Polar software */

	fprintf(fp,"ActiveLimit=0\r\n");

	/*
	   MaxHR, RestHR are not recorded in the file.  They're recorded in
	   the user settings, but instead of downloading them from the watch,
	   we just put default values here.  Someday we may revisit this and
	   set up a way for users to have a .s710rc configuration file or
	   something like that.  That way, this sort of thing can be read
	   from the user's config file.
	*/

	fprintf(fp,"MaxHR=%d\r\n",S725_HRM_MAX_HR);
	fprintf(fp,"RestHR=%d\r\n",S725_HRM_REST_HR);
	fprintf(fp,"StartDelay=0\r\n");

	/* VO2Max, Weight - see above. */

	fprintf(fp,"VO2max=50\r\n");
	fprintf(fp,"Weight=%d\r\n",
			(w->units.system == S725_UNITS_ENGLISH) ? 150 : 70);

	/* Note */

	fprintf(fp,"\r\n[Note]\r\n");

	/* IntTimes */

	fprintf(fp,"\r\n[IntTimes]\r\n");

	for ( i = 0; i < w->laps; i++ ) {

		/*
		   This is really annoying, but we actually have to compute the
		   value of the minimum heart rate for each lap.  We can only do
		   this if we know the recording interval (which we should always
		   know).
		*/

		lap_min_hr = 0;

		if ( w->recording_interval != 0 ) {
			lap_start_sample = w->lap_data[i].split.seconds +
				60 * (w->lap_data[i].split.minutes +
					  60 * w->lap_data[i].split.hours);
			lap_end_sample = w->lap_data[i].cumulative.seconds +
				60 * (w->lap_data[i].cumulative.minutes +
					  60 * w->lap_data[i].cumulative.hours);

			lap_start_sample /= w->recording_interval;
			lap_end_sample /= w->recording_interval;

			lap_start_sample = lap_end_sample - lap_start_sample;

			if ( lap_end_sample >= w->samples ) lap_end_sample = w->samples - 1;
			if ( lap_start_sample < 0 ) lap_start_sample = 0;

			lap_min_hr = 255;
			for ( j = lap_start_sample; j <= lap_end_sample; j++ ) {
				if ( w->hr_data[j] != 0 && w->hr_data[j] < lap_min_hr ) {
					lap_min_hr = w->hr_data[j];
				}
			}
		}

		fprintf(fp,"%02d:%02d:%02d.%d\t%d\t%d\t%d\t%d\r\n",
				w->lap_data[i].cumulative.hours,
				w->lap_data[i].cumulative.minutes,
				w->lap_data[i].cumulative.seconds,
				w->lap_data[i].cumulative.tenths,
				w->lap_data[i].lap_hr,
				lap_min_hr,
				w->lap_data[i].avg_hr,
				w->lap_data[i].max_hr);
		fprintf(fp,"2\t0\t0\t%d\t%d\t%d\r\n",
				(int)(w->lap_data[i].speed * 10.0/16.0 + 0.5),
				w->lap_data[i].cad,
				w->lap_data[i].alt);
		fprintf(fp,"0\t0\t0\t%d\t%d\r\n",
				w->lap_data[i].ascent,
				w->lap_data[i].distance);
		fprintf(fp,"0\t0\t%d\t%d\t0\t0\r\n",
				w->lap_data[i].power.power,
				w->lap_data[i].temp);
		fprintf(fp,"0\t0\t0\t0\t0\t0\r\n");
	}

	/* IntNotes */

	fprintf(fp,"\r\n[IntNotes]\r\n");

	/* ExtraData */

	fprintf(fp,"\r\n[ExtraData]\r\n");

	/* Summary-123 */

	fprintf(fp,"\r\n[Summary-123]\r\n");

	/* We'll need this a few times. */

	sampled_seconds = (w->samples - 1) * w->recording_interval;

	for ( i = 0; i < 3; i++ ) {

		/*
		   We actually have to compute these values.  Actually, one of them
		   shouldn't need to be computed, but while we're at it, why not.
		*/

		above_max      = 0;
		upper_to_max   = 0;
		lower_to_upper = 0;
		rest_to_lower  = 0;
		below_rest     = 0;

		/* We'll need this a few times. */

		/*
		   We start at sample 1.  Each sample covers the preceding five
		   seconds.
		*/

		for ( j = 1; j < w->samples; j++ ) {
			if      ( w->hr_data[j] > S725_HRM_MAX_HR )      above_max++;
			else if ( w->hr_data[j] > w->hr_limit[i].upper ) upper_to_max++;
			else if ( w->hr_data[j] > w->hr_limit[i].lower ) lower_to_upper++;
			else if ( w->hr_data[j] > S725_HRM_REST_HR )     rest_to_lower++;
			else                                             below_rest++;
		}

		/* Now we have the number of samples in each of the five ranges. */

		fprintf(fp,"%d\t%d\t%d\t%d\t%d\t%d\r\n",
				sampled_seconds,
				above_max * w->recording_interval,
				upper_to_max * w->recording_interval,
				lower_to_upper * w->recording_interval,
				rest_to_lower * w->recording_interval,
				below_rest * w->recording_interval);
		fprintf(fp,"%d\t%d\t%d\t%d\r\n",
				S725_HRM_MAX_HR,
				w->hr_limit[i].upper,
				w->hr_limit[i].lower,
				S725_HRM_REST_HR);
	}
	fprintf(fp,"0\t%d\r\n",w->samples-1);

	/* Summary-TH - see comments below. */

	fprintf(fp,"\r\n[Summary-TH]\r\n");
	fprintf(fp,"%d\t0\t%d\t0\t0\t0\r\n",sampled_seconds,sampled_seconds);
	fprintf(fp,"%d\t0\t0\t%d\r\n",S725_HRM_MAX_HR,S725_HRM_REST_HR);
	fprintf(fp,"0\t%d\r\n",w->samples-1);

	/*
	   HRZones - these quantities are set by the user in the Polar software.
	   Fill in with an "empty" set of values.
	*/

	fprintf(fp,"\r\n[HRZones]\r\n");
	fprintf(fp,"0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n");

	/* Trip */

	fprintf(fp,"\r\n[Trip]\r\n");
	fprintf(fp,"%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n%d\r\n",
			w->exercise_distance,
			w->ascent,
			w->duration.seconds +
			60 * (w->duration.minutes + 60 * w->duration.hours),
			w->avg_alt,
			w->max_alt,
			(int)(w->avg_speed * 10.0/16.0 + 0.5),
			(int)(w->max_speed * 10.0/16.0 + 0.5),
			w->odometer);

	/* HRData */

	fprintf(fp,"\r\n[HRData]\r\n");

	for ( i = 0; i < w->samples; i++ ) {
		fprintf(fp,"%d",w->hr_data[i]);
		if ( S725_HAS_SPEED(mode) ) {
			fprintf(fp,"\t%d",(int)(w->speed_data[i] * 10.0 / 16.0 + 0.5));
		}
		if ( S725_HAS_CADENCE(mode) ) {
			fprintf(fp,"\t%d",w->cad_data[i]);
		}
		if ( S725_HAS_ALTITUDE(mode) ) {
			fprintf(fp,"\t%d",w->alt_data[i]);
		}
		if ( S725_HAS_POWER(mode) ) {
			fprintf(fp,"\t%d\t%d",
					w->power_data[i].power,
					(w->power_data[i].pedal_index << 7) +
					(w->power_data[i].lr_balance >> 1));
		}
		fprintf(fp,"\r\n");
	}

	/* That's all, folks. */

	fflush(fp);
}

/*
 * Print workout in TCX format
 *
 * For a specification, see:
 * https://www8.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd
 *
 */
void
workout_print_tcx(workout_t *w, FILE *fp)
{
	char buf[BUFSIZ];
	int i, j, count;
	int count_after_end;
	unsigned long sum_altitude;
	unsigned long sum_cadence;
	unsigned long sum_speed;
	unsigned long sum_power;

	/* sanity checks. */
	if (w == NULL || fp == NULL) {
		log_error("workout_print_hrm: improper usage(%p,%p)", w, fp);
		return;
	}

	if (w->type == S725_HRM_UNKNOWN) {
		log_error("workout_print_hrm: unknown HRM model");
		return;
	}

	sum_altitude = 0;
	sum_cadence  = 0;
	sum_speed    = 0;
	sum_power    = 0;

	if ( w->alt_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_altitude += w->alt_data[i];
	if ( w->cad_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_cadence  += w->cad_data[i];
	if ( w->speed_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_speed    += w->speed_data[i];
	if ( w->power_data != NULL )
		for ( i = 0; i < w->samples; i++ ) sum_power    += w->power_data[i].power;

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n");
	fprintf(fp, "<TrainingCenterDatabase\n"
			"    xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\"\n"
			"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"    xsi:schemaLocation=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\n"
			"    http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd\">\n");

	fprintf(fp, "<Activities>\n");
	fprintf(fp, "  <Activity Sport=\"Running\">\n");

	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ",
			 localtime(&w->unixtime));
	fprintf(fp, "    <Id>%s</Id>\n", buf);

	if (w->units.distance[0] == 'm') {
		log_error("TODO: implement conversion from miles to metres");
	}

	count = 0;
	count_after_end = 0;
	for (i = 0; i < w->laps; i++) {
		fprintf(fp, "    <Lap StartTime=\"%s\">\n", buf);
		fprintf(fp, "      <TotalTimeSeconds>%.5lf</TotalTimeSeconds>\n",
				w->lap_data[i].split.hours * 3600.0 + w->lap_data[i].split.minutes * 60.0 + w->lap_data[i].split.seconds);
		fprintf(fp, "      <DistanceMeters>%.5lf</DistanceMeters>\n",
				w->lap_data[i].distance * 100.0);
		fprintf(fp, "      <MaximumSpeed>%.5lf</MaximumSpeed>\n", w->max_speed * 1.0);
		fprintf(fp, "      <Calories>%d</Calories>\n", w->total_energy);
		fprintf(fp, "      <AverageHeartRateBpm><Value>60</Value></AverageHeartRateBpm>\n");
		fprintf(fp, "      <MaximumHeartRateBpm><Value>60</Value></MaximumHeartRateBpm>\n");
		fprintf(fp, "      <Intensity>Active</Intensity>\n");
		fprintf(fp, "      <TriggerMethod>Manual</TriggerMethod>\n");
		fprintf(fp, "      <Track>\n");

		int cumulative_seconds = w->lap_data[i].cumulative.hours * 3600
			+ w->lap_data[i].cumulative.minutes * 60
			+ w->lap_data[i].cumulative.seconds;
		int cumulative_seconds_prev = 0;

		if (i > 0)
			cumulative_seconds_prev = w->lap_data[i-1].cumulative.hours * 3600
			+ w->lap_data[i-1].cumulative.minutes * 60
			+ w->lap_data[i-1].cumulative.seconds;

		for (j = 0; j < w->samples; j++) {
			if (i == w->laps - 1)
				if (j * w->recording_interval >= cumulative_seconds)
					count_after_end++;

			/* unless last lap, skip samples after end of lap */
			if (i < w->laps -1)
				if (j * w->recording_interval >= cumulative_seconds)
					continue;

			/* except first lap, skip samples before end of last lap */
			if (i > 0 && j * w->recording_interval < cumulative_seconds_prev)
				continue;

			count++;
			fprintf(fp, "        <Trackpoint>\n");
			fprintf(fp, "        <!-- sample #%d -->\n", j);
			time_t stamp = w->unixtime + j * w->recording_interval;
			strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ",
					 localtime(&stamp));
			fprintf(fp, "          <Time>%s</Time>\n", buf);

			if ( w->alt_data != NULL )
				fprintf(fp, "          <AltitudeMeters>%f</AltitudeMeters>\n", w->alt_data[j] * 1.0);

			if ( w->dist_data != NULL )
				fprintf(fp, "          <DistanceMeters>%f</DistanceMeters>\n", w->dist_data[j] * 1.0);

			if ( w->hr_data != NULL )
				fprintf(fp, "          <HeartRateBpm><Value>%hhu</Value></HeartRateBpm>\n", w->hr_data[j]);

			if ( w->cad_data != NULL )
				fprintf(fp, "          <Cadence>%f</Cadence>\n", w->cad_data[j] * 1.0);

			fprintf(fp, "        </Trackpoint>\n");
		}

		fprintf(fp, "      </Track>\n");
		fprintf(fp, "    </Lap>\n");
	}
	fprintf(fp, "  </Activity>\n");
	fprintf(fp, "</Activities>\n");
	fprintf(fp, "</TrainingCenterDatabase>\n");

	if (count != w->samples)
		fprintf(fp, "<!-- #samples does not match: %d != %d -->\n", count, w->samples);

	fprintf(fp, "<!-- output count: %3d -->\n", count);
	fprintf(fp, "<!-- data samples: %3d -->\n", w->samples);
	fprintf(fp, "<!-- after end:    %3d -->\n", count_after_end);

	fflush(fp);
}
