/* workout_int.h - workout internal types */

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

#ifndef WORKOUT_INT_H
#define WORKOUT_INT_H

#include <time.h>

#define S725_HAS_FIELD(x,y)   (((x) & S725_MODE_##y) != 0)
#define S725_HAS_CADENCE(x)   S725_HAS_FIELD(x,CADENCE)
#define S725_HAS_POWER(x)     S725_HAS_FIELD(x,POWER)
#define S725_HAS_SPEED(x)     S725_HAS_FIELD(x,SPEED)
#define S725_HAS_ALTITUDE(x)  S725_HAS_FIELD(x,ALTITUDE)
#define S725_HAS_SPEED1(x)    S725_HAS_FIELD(x,SPEED1)
#define S725_HAS_SPEED2(x)    S725_HAS_FIELD(x,SPEED2)

/* constants */
#define S725_SPEED_KPH       "kph"
#define S725_SPEED_MPH       "mph"
#define S725_DISTANCE_KM     "km"
#define S725_DISTANCE_MI     "mi"
#define S725_ALTITUDE_M      "m"
#define S725_ALTITUDE_FT     "ft"
#define S725_TEMPERATURE_C   "C"
#define S725_TEMPERATURE_F   "F"

#define S725_MODE_HEART_RATE  0
#define S725_MODE_ALTITUDE    2
#define S725_MODE_CADENCE     4
#define S725_MODE_POWER       8
#define S725_MODE_SPEED1      16
#define S725_MODE_SPEED2      32
#define S725_MODE_SPEED      (S725_MODE_SPEED1 | S725_MODE_SPEED2)

#define S725_AM_PM_MODE_UNSET 0
#define S725_AM_PM_MODE_SET   1
#define S725_AM_PM_MODE_PM    2

#define S725_HEADER_SIZE_S625X 130
#define S725_HEADER_SIZE_S725  109
#define S725_HEADER_SIZE_S610   78

#define S725_HRM_MAX_HR  200
#define S725_HRM_REST_HR  60

typedef struct S725_Time {
	int    hours;
	int    minutes;
	int    seconds;
	int    tenths;
} S725_Time;

typedef char S725_Label[8];

typedef enum {
	S725_HRM_AUTO    =  0,
	S725_HRM_S610    = 11,  /* same as in hrm files */
	S725_HRM_S725    = 12,  /* same as in hrm files */
	S725_HRM_S810    = 13,  /* same as in hrm files */
	S725_HRM_S625X   = 22,  /* same as in hrm files */
	S725_HRM_UNKNOWN = 255
} S725_HRM_Type;

typedef enum {
	S725_UNITS_METRIC,
	S725_UNITS_ENGLISH
} S725_Units;

typedef enum {
	S725_HT_SHOW_LIMITS,
	S725_HT_STORE_LAP,
	S725_HT_SWITCH_DISP
} S725_Heart_Touch;

typedef enum {
	S725_RECORD_INT_05,
	S725_RECORD_INT_15,
	S725_RECORD_INT_60
} S725_Recording_Interval;

typedef enum {
	S725_INTERVAL_MODE_OFF = 0,
	S725_INTERVAL_MODE_ON  = 1
} S725_Interval_Mode;

/* basic types */
typedef unsigned char  S725_Heart_Rate;
typedef unsigned char  S725_Cadence;
typedef short          S725_Altitude;
typedef unsigned short S725_Speed;     /* 1/16ths of a km/hr */
typedef float          S725_Distance;

typedef struct S725_Power {
	unsigned short power;
	unsigned char  lr_balance;
	unsigned char  pedal_index;
} S725_Power;

typedef char S725_Temperature;

typedef struct S725_HR_Limit {
	S725_Heart_Rate  lower;
	S725_Heart_Rate  upper;
} S725_HR_Limit;

/* exercise data */
typedef struct exercise_t {
	unsigned int            which;
	S725_Label              label;
	S725_Time               timer[3];
	S725_HR_Limit           hr_limit[3];
	S725_Time               recovery_time;
	S725_Heart_Rate         recovery_hr;
} exercise_t;

/* lap data */
typedef struct lap_data_t {
	S725_Time        split;
	S725_Time        cumulative;
	S725_Heart_Rate  lap_hr;
	S725_Heart_Rate  avg_hr;
	S725_Heart_Rate  max_hr;
	S725_Altitude    alt;
	S725_Altitude    ascent;
	S725_Altitude    cumul_ascent;
	S725_Temperature temp;
	S725_Cadence     cad;
	int              distance;
	int              cumul_distance;
	S725_Speed       speed;
	S725_Power       power;
} lap_data_t;

/* units info */
typedef struct units_data_t {
	S725_Units       system;       /* S725_UNITS_METRIC or S725_UNITS_ENGLISH */
	const char      *altitude;     /* "m" or "ft" */
	const char      *speed;        /* "kph" or "mph" */
	const char      *distance;     /* "km" or "mi" */
	const char      *temperature;  /* "C" or "F" */
} units_data_t;

/* a single workout */
struct workout_t {
	S725_HRM_Type           type;
	struct tm               date;
	int                     ampm;
	time_t                  unixtime;
	int                     user_id;
	S725_Interval_Mode      interval_mode;
	int                     exercise_number;
	S725_Label              exercise_label;
	S725_Time               duration;
	S725_Heart_Rate         avg_hr;
	S725_Heart_Rate         max_hr;
	int                     bytes;
	int                     laps;
	int                     manual_laps;
	int                     samples;
	units_data_t            units;
	int                     mode;
	int                     recording_interval;
	int                     filtered;
	S725_HR_Limit           hr_limit[3];
	S725_Time               hr_zone[3][3];
	S725_Time               bestlap_split;
	S725_Time               cumulative_exercise;
	S725_Time               cumulative_ride;
	int                     odometer;
	int                     exercise_distance;
	S725_Cadence            avg_cad;
	S725_Cadence            max_cad;
	S725_Altitude           min_alt;
	S725_Altitude           avg_alt;
	S725_Altitude           max_alt;
	S725_Temperature        min_temp;
	S725_Temperature        avg_temp;
	S725_Temperature        max_temp;
	S725_Altitude           ascent;
	S725_Power              avg_power;
	S725_Power              max_power;
	S725_Speed              avg_speed;
	S725_Speed              max_speed;
	S725_Speed              median_speed;
	S725_Speed              highest_speed;
	int                     energy;
	int                     total_energy;
	lap_data_t             *lap_data;
	S725_Heart_Rate        *hr_data;
	S725_Altitude          *alt_data;
	S725_Speed             *speed_data;
	S725_Distance          *dist_data;       /* computed from speed_data */
	S725_Cadence           *cad_data;
	S725_Power             *power_data;
};

#endif

