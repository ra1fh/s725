/* workout.c - workout parsing functions */

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

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "log.h"
#include "workout.h"
#include "workout_int.h"
#include "workout_time.h"

static S725_HRM_Type workout_detect_hrm_type(BUF *buf);
static workout_t * workout_extract(BUF *buf, S725_HRM_Type type);
static int workout_bytes_per_sample(unsigned char bt);
static int workout_bytes_per_lap(S725_HRM_Type type, unsigned char bt, unsigned char bi);
static int workout_header_size(workout_t *w);
static int workout_allocate_sample_space(workout_t *w);
static int workout_get_recording_interval(unsigned char b);
static void workout_read_preamble(workout_t *w, BUF *buf);
static void workout_read_date(workout_t *w, BUF *buf);
static void workout_read_duration(workout_t *w, BUF *buf, size_t offset);
static void workout_read_units(workout_t *w, BUF *buf);
static void workout_read_recording_interval(workout_t *w, BUF *buf, size_t offset);
static void workout_read_hr_limits(workout_t *w, BUF *buf, size_t offset);
static void workout_read_bestlap_split(workout_t *w, BUF *buf, size_t offset);
static void workout_read_energy(workout_t *w, BUF *buf, size_t offset);
static void workout_read_cumulative_exercise(workout_t *w, BUF *buf, size_t offset);
static void workout_read_ride_info(workout_t *w, BUF *buf, size_t offset);
static int workout_read_laps(workout_t *w, BUF *buf);
static void workout_compute_speed_info(workout_t *w);
static int workout_read_samples(workout_t *w, BUF *buf);
static void workout_label_extract(BUF *buf, size_t offset, S725_Label *label, int bytes);
static char alpha_map(unsigned char c);

/**********************************************************************/

workout_t *
workout_read_buf(BUF *buf, S725_HRM_Type type)
{
	workout_t *w = NULL;
	off_t size;

	if (buf_len(buf) < 2) {
		log_info("workout_read_buf: buffer size too small");
		return NULL;
	}

	size = buf_getshort(buf, 0);

	if (size != buf_len(buf)) {
		log_info("workout_read_buf: len does not match buffer len");
		return NULL;
	}

	if (type == S725_HRM_AUTO) {
		type = workout_detect_hrm_type(buf);
	}

	if (type == S725_HRM_UNKNOWN || buf_get_readerr(buf)) {
		log_info("workout_read_buf: unable to auto-detect HRM type");
		return NULL;
	}

	w = workout_extract(buf, type);

	return w;
}

workout_t *
workout_read(char *filename, S725_HRM_Type type)
{
	workout_t *w = NULL;
	BUF *buf;

	buf = buf_load(filename);
	if (buf) {
		w = workout_read_buf(buf, type);
		buf_free(buf);
	} else {
		log_info("workout_read: load error");
	}

	return w;
}

void
workout_free (workout_t *w)
{
	if (w != NULL) {
		if (w->lap_data)   free(w->lap_data);
		if (w->alt_data)   free(w->alt_data);
		if (w->speed_data) free(w->speed_data);
		if (w->dist_data)  free(w->dist_data);
		if (w->cad_data)   free(w->cad_data);
		if (w->power_data) free(w->power_data);
		if (w->hr_data)    free(w->hr_data);
		free(w);
	}
}

/**********************************************************************/

/*
 * Attempt to auto-detect the HRM type based on some information in the file.
 * This may not always succeed, but it seems to work relatively well.
 */
static S725_HRM_Type
workout_detect_hrm_type(BUF *buf)
{
	int duration = 0;
	int samples;
	int laps;
	int header = 0;
	int bps = 0;
	int bpl = 0;
	int ri;

	if (buf_len(buf) <= 37) {
		log_error("workout_detect_hrm_type: buffer size too small");
		return S725_HRM_UNKNOWN;
	}

	if (buf_getc(buf, 34) == 0 && buf_getc(buf, 36) == 251) {
		return S725_HRM_S610;
	}

	if (buf_getc(buf, 37) == 251 &&
		(buf_getc(buf, 35) == 0 || buf_getc(buf, 35) == 48)) {
		/* this is either an s725 or s625x or...? */

		if ((ri = workout_get_recording_interval(buf_getc(buf, 27))) != 0) {
			/* compute the number of bytes per sample and per lap */
			bps = workout_bytes_per_sample(buf_getc(buf, 26));
			bpl = workout_bytes_per_lap(S725_HRM_UNKNOWN, buf_getc(buf, 26),
										buf_getc(buf, 23));

			/* obtain the number of laps and samples in the file */
			duration = buf_getbcd(buf, 16)
				+ 60 * (buf_getbcd(buf, 17) + 60 * buf_getbcd(buf, 18));
			samples = duration / ri + 1;
			laps = buf_getbcd(buf, 21);

			/* now compute the size of the file header */
			header = buf_len(buf) - samples * bps - laps * bpl;

			/*
			 * based on the header size, we can make a guess at the
			 * HRM type. note that this will NOT work if the file was
			 * truncated due to the watch memory filling up before
			 * recording was stopped.
			 *
			 * assume it's an S725 unless the header size matches
			 * the S625x header size.
			 */
			if (header == S725_HEADER_SIZE_S625X) {
				return S725_HRM_S625;
			} else {
				return S725_HRM_S725;
			}
		}
	}

	return S725_HRM_UNKNOWN;
}

static workout_t *
workout_extract(BUF *buf, S725_HRM_Type type)
{
	workout_t *w = NULL;
	int ok = 1;

	if ((w = calloc(1, sizeof(workout_t))) == NULL) {
		log_info("workout_extract: calloc: %s", strerror(errno));
		return NULL;
	}

	/* Define the type of the HRM */
	w->type = type;

	/* Now extract the header data */
	workout_read_preamble(w, buf);
	workout_read_date(w, buf);
	workout_read_duration(w, buf, 15);

	if (buf_get_readerr(buf)) {
		log_info("workout_extract: readerr after header (%d > %d)",
				 buf_get_readerr_offset(buf), buf_len(buf));
		workout_free(w);
		return NULL;
	}

	w->avg_hr        = buf_getc(buf, 19);
	w->max_hr        = buf_getc(buf, 20);
	w->laps          = buf_getbcd(buf, 21);
	w->manual_laps   = buf_getbcd(buf, 22);
	w->interval_mode = buf_getc(buf, 23);
	w->user_id       = buf_getbcd(buf, 24);

	workout_read_units(w, buf);

	if (buf_get_readerr(buf)) {
		log_info("workout_extract: readerr after units (%d > %d)",
				 buf_get_readerr_offset(buf), buf_len(buf));
		workout_free(w);
		return NULL;
	}

	/* recording mode and interval */
	if (w->type == S725_HRM_S610) {
		w->mode = 0;
		workout_read_recording_interval  (w, buf, 26);
		workout_read_hr_limits           (w, buf, 28);
		workout_read_bestlap_split       (w, buf, 65);
		workout_read_energy              (w, buf, 69);
		workout_read_cumulative_exercise (w, buf, 75);
	} else {
		w->mode = buf_getc(buf, 26);
		workout_read_recording_interval  (w, buf, 27);
		workout_read_hr_limits           (w, buf, 29);
		workout_read_bestlap_split       (w, buf, 66);
		workout_read_energy              (w, buf, 70);
		workout_read_cumulative_exercise (w, buf, 76);
		workout_read_ride_info           (w, buf, 79);
	}

	if (buf_get_readerr(buf)) {
		log_info("workout_extract: readerr after mode (%d > %d)",
				 buf_get_readerr_offset(buf), buf_len(buf));
		workout_free(w);
		return NULL;
	}

	ok = workout_read_laps(w, buf);
	if (buf_get_readerr(buf)) {
		log_info("workout_extract: readerr after read laps (%d > %d)",
				 buf_get_readerr_offset(buf), buf_len(buf));
		workout_free(w);
		return NULL;
	}
	if (!ok) {
		workout_free(w);
		return NULL;
	}

	ok = workout_read_samples(w, buf);
	if (buf_get_readerr(buf)) {
		log_info("workout_extract: readerr after read samples (%d > %d)",
				 buf_get_readerr_offset(buf), buf_len(buf));
		workout_free(w);
		return NULL;
	}
	if (!ok) {
		workout_free(w);
		return NULL;
	}

	workout_compute_speed_info(w);

	return w;
}

static void
workout_read_preamble(workout_t *w, BUF *buf)
{
	if (buf_len(buf) <= 10) {
		log_error("workout_read_preamble: buffer too small");
		return /* TODO: ERROR */;
	}

	/* number of bytes in the buffer (including these two) */
	w->bytes = buf_getc(buf, 0) + (buf_getc(buf, 1) << 8);

	w->exercise_number = buf_getc(buf, 2);
	if (w->exercise_number > 0 && w->exercise_number <= 5) {
		workout_label_extract(buf, 3, &w->exercise_label, 7);
	} else {
		strncpy(w->exercise_label, "<empty>", sizeof(w->exercise_label) - 1);
		w->exercise_label[sizeof(w->exercise_label) - 1] = '\0';
	}
}

static void
workout_read_date(workout_t *w, BUF *buf)
{
	/* date of workout */
	w->date.tm_sec   = buf_getbcd(buf, 10);
	w->date.tm_min   = buf_getbcd(buf, 11);
	w->date.tm_hour  = buf_getbcd(buf, 12) & 0x7f;

	/* PATCH for AM/PM mode detection from Berend Ozceri */
	w->ampm = S725_AM_PM_MODE_UNSET;
	if (buf_getc(buf, 13) & 0x80) w->ampm |= S725_AM_PM_MODE_SET;
	if (buf_getc(buf, 12) & 0x80) w->ampm |= S725_AM_PM_MODE_PM;

	w->date.tm_hour += (buf_getc(buf, 13) & 0x80) ?                       /* am/pm mode?   */
		((buf_getc(buf, 12) & 0x80) ? ((w->date.tm_hour < 12) ? 12 : 0) : /* yes, pm set   */
		 ((w->date.tm_hour >= 12) ? -12 : 0)) :                           /* yes, pm unset */
		0;                                                                /* no            */

	w->date.tm_mon   = (buf_getc(buf, 15) & 0x0f) - 1;
	w->date.tm_mday  = buf_getbcd(buf, 13) & 0x7f;
	w->date.tm_year  = 100 + buf_getbcd(buf, 14);
	w->date.tm_isdst = -1; /* Daylight savings time not known yet? */
	w->unixtime = mktime(&w->date);
}


static void
workout_read_duration(workout_t *w, BUF *buf, size_t offset)
{
	w->duration.tenths  = buf_getc(buf, offset + 0) >> 4;
	w->duration.seconds = buf_getbcd(buf, offset + 1);
	w->duration.minutes = buf_getbcd(buf, offset + 2);
	w->duration.hours   = buf_getbcd(buf, offset + 3);
}


static void
workout_read_units(workout_t *w, BUF *buf)
{
	unsigned char b = buf_getc(buf, 25);
	if (b & 0x02) { /* english */
		w->units.system      = S725_UNITS_ENGLISH;
		w->units.altitude    = S725_ALTITUDE_FT;
		w->units.speed       = S725_SPEED_MPH;
		w->units.distance    = S725_DISTANCE_MI;
		w->units.temperature = S725_TEMPERATURE_F;
	} else {         /* metric */
		w->units.system      = S725_UNITS_METRIC;
		w->units.altitude    = S725_ALTITUDE_M;
		w->units.speed       = S725_SPEED_KPH;
		w->units.distance    = S725_DISTANCE_KM;
		w->units.temperature = S725_TEMPERATURE_C;
	}
}

static int
workout_get_recording_interval(unsigned char b)
{
	int ri = 0;

	switch (b) {
	case 0:  ri = 5;  break;
	case 1:  ri = 15; break;
	case 2:  ri = 60; break;
	default:          break;
	}

	return ri;
}

static void
workout_read_recording_interval(workout_t *w, BUF *buf, size_t offset)
{
	unsigned char b = buf_getc(buf, offset);
	w->recording_interval = workout_get_recording_interval(b);
}

static void
workout_read_hr_limits(workout_t *w, BUF *buf, size_t offset)
{
	/* HR limits */
	w->hr_limit[0].lower     = buf_getc(buf, offset + 0);
	w->hr_limit[0].upper     = buf_getc(buf, offset + 1);
	w->hr_limit[1].lower     = buf_getc(buf, offset + 2);
	w->hr_limit[1].upper     = buf_getc(buf, offset + 3);
	w->hr_limit[2].lower     = buf_getc(buf, offset + 4);
	w->hr_limit[2].upper     = buf_getc(buf, offset + 5);

	/* time below, within, above hr limits 1 */
	w->hr_zone[0][0].tenths  = 0;
	w->hr_zone[0][0].seconds = buf_getbcd(buf, offset +  9);  /* Below zone 1, seconds */
	w->hr_zone[0][0].minutes = buf_getbcd(buf, offset + 10);  /* Below zone 1, minutes */
	w->hr_zone[0][0].hours   = buf_getbcd(buf, offset + 11);  /* Below zone 1, hours   */
	w->hr_zone[0][1].seconds = buf_getbcd(buf, offset + 12);  /* Within zone 1, seconds */
	w->hr_zone[0][1].minutes = buf_getbcd(buf, offset + 13);  /* Within zone 1, minutes */
	w->hr_zone[0][1].hours   = buf_getbcd(buf, offset + 14);  /* Within zone 1, hours   */
	w->hr_zone[0][2].seconds = buf_getbcd(buf, offset + 15);  /* Above zone 1, seconds */
	w->hr_zone[0][2].minutes = buf_getbcd(buf, offset + 16);  /* Above zone 1, minutes */
	w->hr_zone[0][2].hours   = buf_getbcd(buf, offset + 17);  /* Above zone 1, hours   */

	/* time below, within, above hr limits 2 */
	w->hr_zone[1][0].tenths  = 0;
	w->hr_zone[1][0].seconds = buf_getbcd(buf, offset + 18);  /* Below zone 2, seconds */
	w->hr_zone[1][0].minutes = buf_getbcd(buf, offset + 19);  /* Below zone 2, minutes */
	w->hr_zone[1][0].hours   = buf_getbcd(buf, offset + 20);  /* Below zone 2, hours   */
	w->hr_zone[1][1].seconds = buf_getbcd(buf, offset + 21);  /* Within zone 2, seconds */
	w->hr_zone[1][1].minutes = buf_getbcd(buf, offset + 22);  /* Within zone 2, minutes */
	w->hr_zone[1][1].hours   = buf_getbcd(buf, offset + 23);  /* Within zone 2, hours   */
	w->hr_zone[1][2].seconds = buf_getbcd(buf, offset + 24);  /* Above zone 2, seconds */
	w->hr_zone[1][2].minutes = buf_getbcd(buf, offset + 25);  /* Above zone 2, minutes */
	w->hr_zone[1][2].hours   = buf_getbcd(buf, offset + 26);  /* Above zone 2, hours   */

	/* time below, within, above hr limits 3 */
	w->hr_zone[2][0].tenths  = 0;
	w->hr_zone[2][0].seconds = buf_getbcd(buf, offset + 27);  /* Below zone 3, seconds */
	w->hr_zone[2][0].minutes = buf_getbcd(buf, offset + 28);  /* Below zone 3, minutes */
	w->hr_zone[2][0].hours   = buf_getbcd(buf, offset + 29);  /* Below zone 3, hours   */
	w->hr_zone[2][1].seconds = buf_getbcd(buf, offset + 30);  /* Within zone 3, seconds */
	w->hr_zone[2][1].minutes = buf_getbcd(buf, offset + 31);  /* Within zone 3, minutes */
	w->hr_zone[2][1].hours   = buf_getbcd(buf, offset + 32);  /* Within zone 3, hours   */
	w->hr_zone[2][2].seconds = buf_getbcd(buf, offset + 33);  /* Above zone 3, seconds */
	w->hr_zone[2][2].minutes = buf_getbcd(buf, offset + 34);  /* Above zone 3, minutes */
	w->hr_zone[2][2].hours   = buf_getbcd(buf, offset + 35);  /* Above zone 3, hours   */
}


static void
workout_read_bestlap_split(workout_t *w, BUF *buf, size_t offset)
{
	w->bestlap_split.tenths  = buf_getc(buf, offset + 0) >> 4;
	w->bestlap_split.seconds = buf_getbcd(buf, offset + 1);
	w->bestlap_split.minutes = buf_getbcd(buf, offset + 2);
	w->bestlap_split.hours   = buf_getbcd(buf, offset + 3);
}

static void
workout_read_energy(workout_t *w, BUF *buf, size_t offset)
{
	w->energy =
		  buf_getbcd(buf, offset + 0) / 10
		+ buf_getbcd(buf, offset + 1) * 10
		+ buf_getbcd(buf, offset + 2) * 1000;
	w->total_energy =
		  buf_getbcd(buf, offset + 3)
		+ buf_getbcd(buf, offset + 4) * 100
		+ buf_getbcd(buf, offset + 5) * 10000;
}

static void
workout_read_cumulative_exercise(workout_t *w, BUF *buf, size_t offset)
{
	w->cumulative_exercise.tenths  = 0;
	w->cumulative_exercise.seconds = 0;
	w->cumulative_exercise.minutes = buf_getbcd(buf, offset + 2);
	w->cumulative_exercise.hours   = buf_getbcd(buf, offset + 0) + buf_getbcd(buf, offset + 1) * 100;
}

static void
workout_read_ride_info(workout_t *w, BUF *buf, size_t offset)
{
	w->cumulative_ride.tenths      = 0;
	w->cumulative_ride.seconds     = 0;
	w->cumulative_ride.minutes     = buf_getbcd(buf, offset + 2);
	w->cumulative_ride.hours       = buf_getbcd(buf, offset + 0) + buf_getbcd(buf, offset + 1) * 100;

	w->odometer =
		  buf_getbcd(buf, offset + 5) * 10000
		+ buf_getbcd(buf, offset + 4) * 100
		+ buf_getbcd(buf, offset + 3);


	/* exercise distance */
	w->exercise_distance = buf_getc(buf, offset + 6) + (buf_getc(buf, offset + 7) << 8);

	/* avg and max speed */
	w->avg_speed = buf_getc(buf, offset + 8) | ((buf_getc(buf, offset + 9) & 0xfUL) << 8);
	w->max_speed = (buf_getc(buf, offset + 10) << 4) | (buf_getc(buf, offset + 9) >> 4);

	/* avg, max cadence */
	w->avg_cad  = buf_getc(buf, offset + 11);
	w->max_cad  = buf_getc(buf, offset + 12);

	/* min, avg, max temperature */
	if (w->units.system == S725_UNITS_ENGLISH) {
		/* English units */
		w->min_temp = buf_getc(buf, offset + 19);
		w->avg_temp = buf_getc(buf, offset + 20);
		w->max_temp = buf_getc(buf, offset + 21);

	} else {
		/* Metric units */
		w->min_temp = buf_getc(buf, offset + 19) & 0x7f;
		w->min_temp = (buf_getc(buf, offset + 19) & 0x80) ? w->min_temp : - w->min_temp;

		w->avg_temp = buf_getc(buf, offset + 20) & 0x7f;
		w->avg_temp = (buf_getc(buf, offset + 20) & 0x80) ? w->avg_temp : - w->avg_temp;

		w->max_temp = buf_getc(buf, offset + 21) & 0x7f;
		w->max_temp = (buf_getc(buf, offset + 21) & 0x80) ? w->max_temp : - w->max_temp;
	}

	/* altitude, ascent */
	w->min_alt  = buf_getc(buf, offset + 13) + ((buf_getc(buf, offset + 14) & 0x7f)<<8);
	w->min_alt  = (buf_getc(buf, offset + 14) & 0x80) ? w->min_alt : - w->min_alt;

	w->avg_alt  = buf_getc(buf, offset + 15) + ((buf_getc(buf, offset + 16) & 0x7f)<<8);
	w->avg_alt  = (buf_getc(buf, offset + 16) & 0x80) ? w->avg_alt : - w->avg_alt;

	w->max_alt  = buf_getc(buf, offset + 17) + ((buf_getc(buf, offset + 18) & 0x7f)<<8);
	w->max_alt  = (buf_getc(buf, offset + 18) & 0x80) ? w->max_alt : - w->max_alt;

	w->ascent   = (buf_getc(buf, offset + 23) << 8) + buf_getc(buf, offset + 22);

	/* avg, max power data */
	w->avg_power.power       = buf_getc(buf, offset + 24) + ((buf_getc(buf, offset + 25) & 0x0f) << 8);
	w->max_power.power       =(buf_getc(buf, offset + 25) >> 4) + (buf_getc(buf, offset + 26) << 4);
	w->avg_power.pedal_index = buf_getc(buf, offset + 27);
	w->max_power.pedal_index = buf_getc(buf, offset + 28);
	w->avg_power.lr_balance  = buf_getc(buf, offset + 29);
	/* there is no max_power LR balance */
	w->max_power.lr_balance  = 0;
}

/*
 * Extract the lap data.
 */
static int
workout_read_laps(workout_t *w, BUF *buf)
{
	S725_Distance prev_lap_dist;
	S725_Altitude prev_lap_ascent;
	int offset;
	int lap_size;
	int hdr_size;
	lap_data_t *l;
	int i;

	prev_lap_ascent = 0;
	prev_lap_dist = 0;
	lap_size = workout_bytes_per_lap(w->type, w->mode, w->interval_mode);
	hdr_size = workout_header_size(w);

	if (w->laps <=0 || w->laps > 1024) {
		log_info("workout_read_laps: invalid number of laps (%d)", w->laps);
		return 0;
	}

	w->lap_data = calloc(w->laps, sizeof(lap_data_t));

	if (w->lap_data == NULL) {
		log_info("workout_read_laps: calloc (%s)", strerror(errno));
		return 0;
	}

	for (i = 0; i < w->laps; i++) {
		/* position to the start of the lap */
		offset = hdr_size + i * lap_size;
		l = &w->lap_data[i];

		/* timestamp (split) */
		l->cumulative.hours   = buf_getc(buf, offset + 2);
		l->cumulative.minutes = buf_getc(buf, offset + 1) & 0x3f;
		l->cumulative.seconds = buf_getc(buf, offset + 0) & 0x3f;
		l->cumulative.tenths  = ((buf_getc(buf, offset + 1) & 0xc0)>>4) |
			((buf_getc(buf, offset) & 0xc0)>>6);

		if (i == 0)
			memcpy(&l->split, &l->cumulative, sizeof(S725_Time));
		else
			workout_time_diff(&w->lap_data[i-1].cumulative, &l->cumulative, &l->split);

		/* heart rate data */
		l->lap_hr = buf_getc(buf, offset + 3);
		l->avg_hr = buf_getc(buf, offset + 4);
		l->max_hr = buf_getc(buf, offset + 5);
		offset += 6;

		/* altitude data */
		if (S725_HAS_ALTITUDE(w->mode)) {
			l->alt = buf_getc(buf, offset) + (buf_getc(buf, offset + 1) << 8) - 512;
			/* This ascent data is cumulative from start of the exercise */
			l->cumul_ascent = buf_getc(buf, offset + 2) + (buf_getc(buf, offset + 3) << 8);
			l->ascent = l->cumul_ascent - prev_lap_ascent;
			prev_lap_ascent = l->cumul_ascent;

			if (w->units.system == S725_UNITS_ENGLISH) {  /* English units */
				l->temp = buf_getc(buf, offset + 4) + 14;
				l->alt *= 5;
			} else {
				l->temp = buf_getc(buf, offset + 4) - 10;
			}

			offset += 5;
		}

		/* bike data */
		if (S725_HAS_SPEED(w->mode)) {
			/* cadence data */
			if (S725_HAS_CADENCE(w->mode)) {
				l->cad  = buf_getc(buf, offset);
				offset += 1;
			}

			/* next 4 bytes are power data */
			if (S725_HAS_POWER(w->mode)) {
				l->power.power = buf_getc(buf, offset) + (buf_getc(buf, offset + 1) << 8);
				l->power.lr_balance  = buf_getc(buf, offset + 3);   /* ??? switched  ??? */
				l->power.pedal_index = buf_getc(buf, offset + 2);   /* ??? with this ??? */
				offset += 4;
			}

			/*
			 * next 4 bytes are distance/speed data
			 * this is cumulative distance from start (at least with S720i)
			 */
			l->cumul_distance = buf_getc(buf, offset) + (buf_getc(buf, offset + 1) << 8);
			l->distance = l->cumul_distance - prev_lap_dist;
			prev_lap_dist = l->cumul_distance;

			/*
			 * The offset + 2 appears to be 4b.4b, where the upper 4b is the
			 * integer portion of the speed and the lower 4b is the fractional
			 * portion in sixteenths.  I'm not sure where the most significant
			 * bits come from, but it might be the high order nibble of the next
			 * byte- lets try that.
			*/
      		l->speed = buf_getc(buf, offset + 2) + ((buf_getc(buf, offset + 3) & 0xf0) << 4);
		}
	}

	return 1;
}

static int
workout_read_samples(workout_t *w, BUF *buf)
{
	int offset;
	int lap_size;
	int sample_size;
	unsigned long accum;
	int ok = 1;
	int i;
	int s;
	int x;

	lap_size = workout_bytes_per_lap(w->type, w->mode, w->interval_mode);
	sample_size = workout_bytes_per_sample(w->mode);
	offset = workout_header_size(w);

	if (offset != 0) {

		/* now add the offset due to laps */
		offset += w->laps * lap_size;

		/* number of samples */
		w->samples = (w->bytes - offset)/sample_size;

		/* allocate memory */
		ok = workout_allocate_sample_space(w);

		/* if we succeeded in allocating the buffers, ok will not be 0 here. */
		if (ok) {
			/* At last, we can extract the samples.  They are in reverse order. */
			for (i = 0; i < w->samples; i++) {
				s = offset + i * sample_size;
				x = w->samples - 1 - i;
				w->hr_data[x] = buf_getc(buf, s);
				s++;

				if (S725_HAS_ALTITUDE(w->mode)) {
					w->alt_data[x] = buf_getc(buf, s) + ((buf_getc(buf, s + 1) & 0x1f) << 8) - 512;
					if (w->units.system == S725_UNITS_ENGLISH) {
						w->alt_data[x] *= 5;
					}
					s += 2;
				}

				if (S725_HAS_SPEED(w->mode)) {
					if (S725_HAS_ALTITUDE(w->mode)) s -= 1;
					w->speed_data[x] = ((buf_getc(buf, s) & 0xe0) << 3) + buf_getc(buf, s + 1);
					s += 2;
					if (S725_HAS_POWER(w->mode)) {
						w->power_data[x].power = buf_getc(buf, s) + (buf_getc(buf, s + 1) << 8);
						w->power_data[x].lr_balance = buf_getc(buf, s + 2);
						w->power_data[x].pedal_index = buf_getc(buf, s + 3);
						s += 4;
					}
					if (S725_HAS_CADENCE(w->mode)) w->cad_data[x] = buf_getc(buf, s);
				}
			}

			if (S725_HAS_SPEED(w->mode)) {
				accum = 0;
				for (i = 0; i < w->samples; i++) {
					w->dist_data[i] = accum / 57600.0;
					accum += w->speed_data[i] * w->recording_interval;
				}
			}

		}
	} else {
		/* we don't know what kind of HRM this is */
		ok = 0;
	}

	return ok;
}

static void
workout_compute_speed_info(workout_t *w)
{
	int i;
	int j;
	float avg;

	/* compute median speed and highest sampled speed */
	if (S725_HAS_SPEED(w->mode)) {
		avg = 0;
		w->highest_speed = 0;
		j = 0;
		for (i = 0; i < w->samples; i++) {
			if (w->speed_data[i] > w->highest_speed)
				w->highest_speed = w->speed_data[i];
			if (w->speed_data[i] > 0) {
				avg = (float)(avg * j + w->speed_data[i])/(j+1);
				j++;
			}
		}
		w->median_speed = avg;
	}
}

static int
workout_header_size(workout_t *w)
{
	int size = 0;

	switch (w->type) {
	case S725_HRM_S610:
		size = S725_HEADER_SIZE_S610;
		break;
	case S725_HRM_S625:
		size = S725_HEADER_SIZE_S625X;
		break;
	case S725_HRM_S725:
		size = S725_HEADER_SIZE_S725;
		break;
	default:
		break;
	}

	return size;
}

static int
workout_bytes_per_lap(S725_HRM_Type type, unsigned char bt, unsigned char bi)
{
	int lap_size = 6;

	/* Compute the number of bytes per lap. */
	if (S725_HAS_ALTITUDE(bt))
		lap_size += 5;
	if (S725_HAS_SPEED(bt)) {
		if (S725_HAS_CADENCE(bt))
			lap_size += 1;
		if (S725_HAS_POWER(bt))
			lap_size += 4;
		lap_size += 4;
	}

	/*
	 * This is Matti Tahvonen's fix for handling laps with interval mode.
	 * Applies to the S625X and the S725/S720i.
	 */
	if (type != S725_HRM_S610 && bi != 0) {
		lap_size += 5;
	}

	return lap_size;
}

static int
workout_bytes_per_sample(unsigned char bt)
{
	int recsiz = 1;

	if (S725_HAS_ALTITUDE(bt))
		recsiz += 2;

	if (S725_HAS_SPEED(bt)) {
		if (S725_HAS_ALTITUDE(bt))
			recsiz -= 1;
		recsiz += 2;
		if (S725_HAS_POWER(bt))
			recsiz += 4;
		if (S725_HAS_CADENCE(bt))
			recsiz += 1;
	}

	return recsiz;
}

static int
workout_allocate_sample_space (workout_t *w)
{
	int ok = 1;

	if (w->samples <= 0 || w->samples > 1048576) {
		log_error("workout_allocate_sample_space: invalid number of samples (%d)", w->samples);
		return 0;
	}

#define  MAKEBUF(a, b)											\
	if ((w->a = calloc(w->samples, sizeof(b))) == NULL) {		\
		log_error("%s: calloc(%d,%ld): %s",					    \
				 #a,w->samples,(long)sizeof(b),strerror(errno));\
		ok = 0;													\
	}

	MAKEBUF(hr_data, S725_Heart_Rate);
	if (S725_HAS_ALTITUDE(w->mode))
		MAKEBUF(alt_data, S725_Altitude);
	if (S725_HAS_SPEED(w->mode)) {
		MAKEBUF(speed_data, S725_Speed);
		MAKEBUF(dist_data, S725_Distance);
		if (S725_HAS_POWER(w->mode))
			MAKEBUF(power_data, S725_Power);
		if (S725_HAS_CADENCE(w->mode))
			MAKEBUF(cad_data, S725_Cadence);
	}

#undef MAKEBUF

	return ok;
}

static void
workout_label_extract(BUF *buf, size_t offset, S725_Label *label, int bytes)
{
	int i;
	char *p = (char*) label;

	for (i = 0; i < bytes && i < (int) sizeof(S725_Label) - 1; i++) {
		*(p + i) = alpha_map(buf_getc(buf, i + offset));
	}

	*(p + i) = 0;
}

static char
alpha_map(unsigned char c)
{
	char a = '?';

	switch (c) {
	case 0: case 1: case 2: case 3: case 4:
	case 5: case 6: case 7: case 8: case 9:
		a = '0' + c;
		break;
	case 10:
		a = ' ';
		break;
	case 11: case 12: case 13: case 14: case 15:
	case 16: case 17: case 18: case 19: case 20:
	case 21: case 22: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30:
	case 31: case 32: case 33: case 34: case 35:
	case 36:
		a = 'A' + c - 11;
		break;
	case 37: case 38: case 39: case 40: case 41:
	case 42: case 43: case 44: case 45: case 46:
	case 47: case 48: case 49: case 50: case 51:
	case 52: case 53: case 54: case 55: case 56:
	case 57: case 58: case 59: case 60: case 61:
	case 62:
		a = 'a' + c - 37;
		break;
	case 63: a = '-'; break;
	case 64: a = '%'; break;
	case 65: a = '/'; break;
	case 66: a = '('; break;
	case 67: a = ')'; break;
	case 68: a = '*'; break;
	case 69: a = '+'; break;
	case 70: a = '.'; break;
	case 71: a = ':'; break;
	case 72: a = '?'; break;
	default:          break;
	}

	return a;
}
