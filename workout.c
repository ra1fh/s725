/*
 * workout parsing functions
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"
#include "workout.h"
#include "workout_int.h"
#include "workout_label.h"
#include "workout_time.h"

int  		workout_bytes_per_lap(S725_HRM_Type type, unsigned char bt, unsigned char bi);
int  		workout_header_size(workout_t *w);
int  		workout_bytes_per_sample(unsigned char bt);
int  		workout_allocate_sample_space(workout_t *w);
static void workout_read_preamble(workout_t *w, unsigned char *buf);
static void workout_read_date(workout_t *w, unsigned char *buf);
static void workout_read_duration(workout_t *w, unsigned char *buf);
static void workout_read_units(workout_t *w, unsigned char b);
static int  workout_get_recording_interval(unsigned char b);
static void workout_read_recording_interval(workout_t *w, unsigned char *buf);
static void workout_read_hr_limits(workout_t *w, unsigned char *buf);
static void workout_read_bestlap_split(workout_t *w, unsigned char *buf);
static void workout_read_energy(workout_t *w, unsigned char *buf);
static void workout_read_cumulative_exercise(workout_t *w, unsigned char *buf);
static void workout_read_ride_info(workout_t *w, unsigned char *buf);
static void workout_read_laps(workout_t *w, unsigned char *buf);
static int  workout_read_samples(workout_t *w, unsigned char *buf);
static void workout_compute_speed_info(workout_t *w);
static workout_t * workout_extract(unsigned char *buf, S725_HRM_Type type);
static S725_HRM_Type workout_detect_hrm_type(unsigned char *buf, unsigned int bytes);

static void
workout_read_preamble(workout_t *w, unsigned char *buf)
{
	/* number of bytes in the buffer (including these two) */
	w->bytes = buf[0] + (buf[1]<<8);

	w->exercise_number = buf[2];
	if (w->exercise_number > 0 && w->exercise_number <= 5) {
		workout_label_extract(&buf[3], &w->exercise_label, 7);
	} else {
		strlcpy(w->exercise_label, "<empty>", sizeof(w->exercise_label));
	}
}

static void
workout_read_date(workout_t *w, unsigned char *buf)
{
	/* date of workout */
	w->date.tm_sec   = BCD(buf[0]);
	w->date.tm_min   = BCD(buf[1]);
	w->date.tm_hour  = BCD(buf[2] & 0x7f);
  
	/* PATCH for AM/PM mode detection from Berend Ozceri */
	w->ampm = S725_AM_PM_MODE_UNSET;
	if (buf[3] & 0x80) w->ampm |= S725_AM_PM_MODE_SET;
	if (buf[2] & 0x80) w->ampm |= S725_AM_PM_MODE_PM;
  
	w->date.tm_hour += (buf[3] & 0x80) ?                      /* am/pm mode?   */
		((buf[2] & 0x80) ? ((w->date.tm_hour < 12) ? 12 : 0) :  /* yes, pm set   */
		 ((w->date.tm_hour >= 12) ? -12 : 0)) :                 /* yes, pm unset */
		0;                                                      /* no            */
  
	w->date.tm_mon   = LNIB(buf[5]) - 1;
	w->date.tm_mday  = BCD(buf[3] & 0x7f);
	w->date.tm_year  = 100 + BCD(buf[4]);
	w->date.tm_isdst = -1; /* Daylight savings time not known yet? */
	w->unixtime = mktime(&w->date);
}


static void
workout_read_duration(workout_t *w, unsigned char *buf)
{
	w->duration.tenths  = UNIB(buf[0]);
	w->duration.seconds = BCD(buf[1]);
	w->duration.minutes = BCD(buf[2]);
	w->duration.hours   = BCD(buf[3]);
}


static void
workout_read_units(workout_t *w, unsigned char b)
{
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
workout_read_recording_interval(workout_t *w, unsigned char *buf)
{
	w->recording_interval = workout_get_recording_interval(buf[0]);
}

static void
workout_read_hr_limits(workout_t *w, unsigned char *buf)
{
	/* HR limits */
	w->hr_limit[0].lower     = buf[0];
	w->hr_limit[0].upper     = buf[1];
	w->hr_limit[1].lower     = buf[2];
	w->hr_limit[1].upper     = buf[3];
	w->hr_limit[2].lower     = buf[4];
	w->hr_limit[2].upper     = buf[5];

	/* time below, within, above hr limits 1 */
	w->hr_zone[0][0].tenths  = 0;
	w->hr_zone[0][0].seconds = BCD(buf[9]);   /* Below zone 1, seconds */
	w->hr_zone[0][0].minutes = BCD(buf[10]);  /* Below zone 1, minutes */
	w->hr_zone[0][0].hours   = BCD(buf[11]);  /* Below zone 1, hours   */
	w->hr_zone[0][1].seconds = BCD(buf[12]);  /* Within zone 1, seconds */
	w->hr_zone[0][1].minutes = BCD(buf[13]);  /* Within zone 1, minutes */
	w->hr_zone[0][1].hours   = BCD(buf[14]);  /* Within zone 1, hours   */
	w->hr_zone[0][2].seconds = BCD(buf[15]);  /* Above zone 1, seconds */
	w->hr_zone[0][2].minutes = BCD(buf[16]);  /* Above zone 1, minutes */
	w->hr_zone[0][2].hours   = BCD(buf[17]);  /* Above zone 1, hours   */

	/* time below, within, above hr limits 2 */
	w->hr_zone[1][0].tenths  = 0;
	w->hr_zone[1][0].seconds = BCD(buf[18]);  /* Below zone 2, seconds */
	w->hr_zone[1][0].minutes = BCD(buf[19]);  /* Below zone 2, minutes */
	w->hr_zone[1][0].hours   = BCD(buf[20]);  /* Below zone 2, hours   */
	w->hr_zone[1][1].seconds = BCD(buf[21]);  /* Within zone 2, seconds */
	w->hr_zone[1][1].minutes = BCD(buf[22]);  /* Within zone 2, minutes */
	w->hr_zone[1][1].hours   = BCD(buf[23]);  /* Within zone 2, hours   */
	w->hr_zone[1][2].seconds = BCD(buf[24]);  /* Above zone 2, seconds */
	w->hr_zone[1][2].minutes = BCD(buf[25]);  /* Above zone 2, minutes */
	w->hr_zone[1][2].hours   = BCD(buf[26]);  /* Above zone 2, hours   */

	/* time below, within, above hr limits 3 */
	w->hr_zone[2][0].tenths  = 0;
	w->hr_zone[2][0].seconds = BCD(buf[27]);  /* Below zone 3, seconds */
	w->hr_zone[2][0].minutes = BCD(buf[28]);  /* Below zone 3, minutes */
	w->hr_zone[2][0].hours   = BCD(buf[29]);  /* Below zone 3, hours   */
	w->hr_zone[2][1].seconds = BCD(buf[30]);  /* Within zone 3, seconds */
	w->hr_zone[2][1].minutes = BCD(buf[31]);  /* Within zone 3, minutes */
	w->hr_zone[2][1].hours   = BCD(buf[32]);  /* Within zone 3, hours   */
	w->hr_zone[2][2].seconds = BCD(buf[33]);  /* Above zone 3, seconds */
	w->hr_zone[2][2].minutes = BCD(buf[34]);  /* Above zone 3, minutes */
	w->hr_zone[2][2].hours   = BCD(buf[35]);  /* Above zone 3, hours   */
}


static void
workout_read_bestlap_split(workout_t *w, unsigned char *buf)
{
	w->bestlap_split.tenths  = UNIB(buf[0]);
	w->bestlap_split.seconds = BCD(buf[1]);
	w->bestlap_split.minutes = BCD(buf[2]);
	w->bestlap_split.hours   = BCD(buf[3]);
}

static void
workout_read_energy(workout_t *w, unsigned char *buf)
{
	w->energy       = BCD(buf[0])/10 + 10 * BCD(buf[1]) + 1000 * BCD(buf[2]);
	w->total_energy = BCD(buf[3]) + 100 * BCD(buf[4]) + 10000 * BCD(buf[5]);
}

static void
workout_read_cumulative_exercise(workout_t *w, unsigned char *buf)
{
	w->cumulative_exercise.tenths  = 0;
	w->cumulative_exercise.seconds = 0;
	w->cumulative_exercise.minutes = BCD(buf[2]);
	w->cumulative_exercise.hours   = BCD(buf[0]) + BCD(buf[1]) * 100;
}

static void
workout_read_ride_info(workout_t *w, unsigned char *buf)
{
	w->cumulative_ride.tenths      = 0;
	w->cumulative_ride.seconds     = 0;
	w->cumulative_ride.minutes     = BCD(buf[2]);
	w->cumulative_ride.hours       = BCD(buf[0]) + BCD(buf[1]) * 100;

	w->odometer = 10000 * BCD(buf[5]) + 100 * BCD(buf[4]) + BCD(buf[3]);

	/* exercise distance */
	w->exercise_distance = buf[6] + (buf[7]<<8);

	/* avg and max speed */
	w->avg_speed = buf[8] | ((buf[9] & 0xfUL) << 8);
	w->max_speed = (buf[10] << 4) | (buf[9] >> 4);

	/* avg, max cadence */
	w->avg_cad  = buf[11];
	w->max_cad  = buf[12];

	/* min, avg, max temperature */
	if (w->units.system == S725_UNITS_ENGLISH) {
		/* English units */
		w->min_temp = buf[19];
		w->avg_temp = buf[20];
		w->max_temp = buf[21];

	} else {
		/* Metric units */
		w->min_temp = buf[19] & 0x7f;
		w->min_temp = (buf[19] & 0x80) ? w->min_temp : - w->min_temp;
    
		w->avg_temp = buf[20] & 0x7f;
		w->avg_temp = (buf[20] & 0x80) ? w->avg_temp : - w->avg_temp;
    
		w->max_temp = buf[21] & 0x7f;
		w->max_temp = (buf[21] & 0x80) ? w->max_temp : - w->max_temp;
	}

	/* altitude, ascent */
	w->min_alt  = buf[13] + ((buf[14] & 0x7f)<<8);
	w->min_alt  = (buf[14] & 0x80) ? w->min_alt : - w->min_alt;

	w->avg_alt  = buf[15] + ((buf[16] & 0x7f)<<8);
	w->avg_alt  = (buf[16] & 0x80) ? w->avg_alt : - w->avg_alt;

	w->max_alt  = buf[17] + ((buf[18] & 0x7f)<<8);
	w->max_alt  = (buf[18] & 0x80) ? w->max_alt : - w->max_alt;
  
	w->ascent   = (buf[23] << 8) + buf[22];

	/* avg, max power data */
	w->avg_power.power       = buf[24] + (LNIB(buf[25]) << 8);
	w->max_power.power       = UNIB(buf[25]) + (buf[26] << 4);
	w->avg_power.pedal_index = buf[27];
	w->max_power.pedal_index = buf[28];
	w->avg_power.lr_balance  = buf[29];
	/* there is no max_power LR balance */
	w->max_power.lr_balance  = 0;
}

/*
 * Extract the lap data. 
 */
static void
workout_read_laps(workout_t *w, unsigned char *buf)
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
	w->lap_data = calloc(w->laps, sizeof(lap_data_t));

	for (i = 0; i < w->laps; i++) {
		/* position to the start of the lap */
		offset = hdr_size + i * lap_size;
		l = &w->lap_data[i];

		/* timestamp (split) */
		l->cumulative.hours   = buf[offset+2];
		l->cumulative.minutes = buf[offset+1] & 0x3f;
		l->cumulative.seconds = buf[offset] & 0x3f;
		l->cumulative.tenths  = ((buf[offset+1] & 0xc0)>>4) | 
			((buf[offset] & 0xc0)>>6);

		if (i == 0) 
			memcpy(&l->split, &l->cumulative, sizeof(S725_Time));
		else
			workout_time_diff(&w->lap_data[i-1].cumulative,&l->cumulative, &l->split);

		/* heart rate data */
		l->lap_hr = buf[offset+3];
		l->avg_hr = buf[offset+4];
		l->max_hr = buf[offset+5];
		offset += 6;

		/* altitude data */
		if (S725_HAS_ALTITUDE(w->mode)) {
			l->alt = buf[offset] + (buf[offset+1]<<8) - 512;
			/* This ascent data is cumulative from start of the exercise */
			l->cumul_ascent = buf[offset+2] + (buf[offset+3]<<8);
			l->ascent = l->cumul_ascent - prev_lap_ascent;
			prev_lap_ascent = l->cumul_ascent;

			if (w->units.system == S725_UNITS_ENGLISH) {  /* English units */
				l->temp = buf[offset+4] + 14;
				l->alt *= 5;
			} else {
				l->temp = buf[offset+4] - 10;
			}

			offset += 5;
		}

		/* bike data */
		if (S725_HAS_SPEED(w->mode)) {
			/* cadence data */
			if (S725_HAS_CADENCE(w->mode)) { 
				l->cad  = buf[offset]; 
				offset += 1; 
			}

			/* next 4 bytes are power data */
			if (S725_HAS_POWER(w->mode)) { 
				l->power.power = buf[offset] + (buf[offset+1]<<8);
				l->power.lr_balance  = buf[offset+3];   /* ??? switched  ??? */
				l->power.pedal_index = buf[offset+2];   /* ??? with this ??? */
				offset += 4;
			}

			/* 
			 * next 4 bytes are distance/speed data
			 * this is cumulative distance from start (at least with S720i)
			 */
			l->cumul_distance = buf[offset] + (buf[offset+1]<<8);
			l->distance = l->cumul_distance - prev_lap_dist;
			prev_lap_dist = l->cumul_distance;

			/* 
			 * The offset + 2 appears to be 4b.4b, where the upper 4b is the
			 * integer portion of the speed and the lower 4b is the fractional
			 * portion in sixteenths.  I'm not sure where the most significant
			 * bits come from, but it might be the high order nibble of the next
			 * byte- lets try that.
			*/
      		l->speed = buf[offset+2] + ((buf[offset+3] & 0xf0) << 4);
		}
	}
}

static int
workout_read_samples(workout_t *w, unsigned char *buf)
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
				w->hr_data[x] = buf[s];
				s++;
	
				if (S725_HAS_ALTITUDE(w->mode)) {
					w->alt_data[x] = buf[s] + ((buf[s+1] & 0x1f)<<8) - 512;
					if (w->units.system == S725_UNITS_ENGLISH) {
						w->alt_data[x] *= 5;
					}
					s += 2;
				}
	
				if (S725_HAS_SPEED(w->mode)) {
					if (S725_HAS_ALTITUDE(w->mode)) s -= 1;
					w->speed_data[x] = ((buf[s] & 0xe0) << 3) + buf[s+1];
					s += 2;
					if (S725_HAS_POWER(w->mode)) { 
						w->power_data[x].power = buf[s] + (buf[s+1]<<8);
						w->power_data[x].lr_balance = buf[s+2];
						w->power_data[x].pedal_index = buf[s+3];
						s += 4;
					}
					if (S725_HAS_CADENCE(w->mode)) w->cad_data[x] = buf[s];
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

/* 
 * don't call this unless you're sure it's going to be ok.
 */
static workout_t *
workout_extract(unsigned char *buf, S725_HRM_Type type)
{
	workout_t *w = NULL;
	int ok = 1;

	if ((w = calloc(1,sizeof(workout_t))) == NULL) {
		log_error("extract_workout: calloc(%ld): %s",
				  (long)sizeof(workout_t),strerror(errno));
		return NULL;
	}

	/* Define the type of the HRM */
	w->type = type;

	/* Now extract the header data */
	workout_read_preamble(w,buf);
	workout_read_date(w,buf+10);
	workout_read_duration(w,buf+15);

	w->avg_hr        = buf[19];
	w->max_hr        = buf[20];
	w->laps          = BCD(buf[21]);
	w->manual_laps   = BCD(buf[22]);
	w->interval_mode = buf[23];
	w->user_id       = BCD(buf[24]);

	workout_read_units(w,buf[25]);

	/* recording mode and interval */
	if (w->type == S725_HRM_S610) {
		w->mode = 0;
		workout_read_recording_interval  (w,buf+26);
		workout_read_hr_limits           (w,buf+28);
		workout_read_bestlap_split       (w,buf+65);
		workout_read_energy              (w,buf+69);
		workout_read_cumulative_exercise (w,buf+75);
	} else {
		w->mode = buf[26];
		workout_read_recording_interval  (w,buf+27);
		workout_read_hr_limits           (w,buf+29);
		workout_read_bestlap_split       (w,buf+66);
		workout_read_energy              (w,buf+70);
		workout_read_cumulative_exercise (w,buf+76);
		workout_read_ride_info           (w,buf+79);
	}

	workout_read_laps(w, buf);
	ok = workout_read_samples(w, buf);

	/* never let a partially allocated workout get through. */
	if (!ok) {
		workout_free(w);
		return NULL;
	}

	workout_compute_speed_info(w);

	return w;
}

/* 
 * Attempt to auto-detect the HRM type based on some information in the file.
 * This may not always succeed, but it seems to work relatively well.
 */
static S725_HRM_Type
workout_detect_hrm_type(unsigned char * buf, unsigned int bytes)
{
	S725_HRM_Type type = S725_HRM_UNKNOWN;
	int duration = 0;
	int samples;
	int laps;
	int header = 0;
	int bps = 0;
	int bpl = 0;
	int ri;

	if (buf[34] == 0 && buf[36] == 251) { 
		/* this is a s610 HRM */
		type = S725_HRM_S610;
	} else if ((buf[35] == 0 || buf[35] == 48) && buf[37] == 251) {
		/* this is either an s725 or s625x or...? */

		if ((ri = workout_get_recording_interval(buf[27])) != 0) {
			/* compute the number of bytes per sample and per lap */
			bps = workout_bytes_per_sample(buf[26]);
			bpl = workout_bytes_per_lap(S725_HRM_UNKNOWN, buf[26], buf[23]);
      
			/* obtain the number of laps and samples in the file */
			duration   = BCD(buf[16])+60*(BCD(buf[17])+60*BCD(buf[18]));
			samples    = duration / ri + 1;
			laps       = BCD(buf[21]);
      
			/* now compute the size of the file header */
			header     = bytes - samples * bps - laps * bpl;
      
			/*
			 * based on the header size, we can make a guess at the
			 * HRM type. note that this will NOT work if the file was
			 * truncated due to the watch memory filling up before
			 * recording was stopped.
			 */
      
			/* 
			 * assume it's an S725 unless the header size matches
			 * the S625x header size.
			 */
			if (header == S725_HEADER_SIZE_S625X) {
				type = S725_HRM_S625X; 
			} else {
				type = S725_HRM_S725;
			}
		}
	}

	return type;
}

workout_t *
workout_read_buf(BUF *buf)
{
	int type;
	workout_t *w = NULL;
	off_t size;

	if (buf_len(buf) < 2) {
		log_error("workout_read_buf: buffer size too small");
		return NULL;
	}

	size = buf_get(buf)[0] + (buf_get(buf)[1]<<8);
	
	if (size != buf_len(buf)) {
		log_error("workout_read_buf: len does not match buffer len");
		return NULL;
	}

	type = workout_detect_hrm_type(buf_get(buf), buf_len(buf));

	if (type == S725_HRM_UNKNOWN) {
		log_error("workout_read_buf: unable to auto-detect HRM type");
		return NULL;
	}

	w = workout_extract(buf_get(buf), type);

	return w;
}

workout_t *
workout_read(char *filename)
{
	workout_t *w = NULL;
	BUF *buf;

	buf = buf_load(filename);
	if (buf)
		w = workout_read_buf(buf);

	return w;
}

int
workout_header_size(workout_t *w)
{
	int size = 0;

	switch (w->type) {
	case S725_HRM_S610:
		size = S725_HEADER_SIZE_S610;
		break;
	case S725_HRM_S625X:
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

int
workout_bytes_per_lap(S725_HRM_Type type, unsigned char bt, unsigned char bi)
{
	int lap_size = 6;

	/* Compute the number of bytes per lap. */
	if (S725_HAS_ALTITUDE(bt))  lap_size += 5;
	if (S725_HAS_SPEED(bt)) {
		if (S725_HAS_CADENCE(bt)) lap_size += 1;
		if (S725_HAS_POWER(bt))   lap_size += 4;
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

int
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

int
workout_allocate_sample_space (workout_t *w)
{
	int ok = 1;

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
