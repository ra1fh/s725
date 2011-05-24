#ifndef __S710_H__
#define __S710_H__

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "buf.h"

/* constants */

#define S710_SPEED_KPH       "kph"
#define S710_SPEED_MPH       "mph"
#define S710_DISTANCE_KM     "km"
#define S710_DISTANCE_MI     "mi"
#define S710_ALTITUDE_M      "m"
#define S710_ALTITUDE_FT     "ft"
#define S710_TEMPERATURE_C   "C"
#define S710_TEMPERATURE_F   "F"

#define S710_REQUEST         0xa3
#define S710_RESPONSE        0x5c

#define S710_USB_VENDOR_ID   0x0da4
#define S710_USB_PRODUCT_ID  1
#define S710_USB_INTERFACE   0

#define S710_MODE_HEART_RATE  0
#define S710_MODE_ALTITUDE    2
#define S710_MODE_CADENCE     4
#define S710_MODE_POWER       8
#define S710_MODE_BIKE1      16
#define S710_MODE_BIKE2      32
#define S710_MODE_SPEED      (S710_MODE_BIKE1 | S710_MODE_BIKE2)

#define S710_AM_PM_MODE_UNSET 0
#define S710_AM_PM_MODE_SET   1
#define S710_AM_PM_MODE_PM    2

#define S710_WORKOUT_HEADER   1
#define S710_WORKOUT_LAPS     2
#define S710_WORKOUT_SAMPLES  4
#define S710_WORKOUT_FULL     7

#define S710_TIC_PLAIN        0
#define S710_TIC_LINES        1
#define S710_TIC_SHADE        2
#define S710_TIC_SHADE_RED    (S710_TIC_SHADE | 4)
#define S710_TIC_SHADE_GREEN  (S710_TIC_SHADE | 8)
#define S710_TIC_SHADE_BLUE   (S710_TIC_SHADE | 16)

#define S710_Y_AXIS_LEFT        -1
#define S710_Y_AXIS_RIGHT        1

#define S710_X_MARGIN         15
#define S710_Y_MARGIN         15

#define S710_BLANK_SAMPLE_LIMIT  2880 /* 4 hours at 5 second intervals. */

#define S710_HEADER_SIZE_S625X 130
#define S710_HEADER_SIZE_S710  109
#define S710_HEADER_SIZE_S610   78

typedef enum {
	S710_PACKET_INDEX_INVALID = -1,
	S710_GET_OVERVIEW = 0,
	S710_GET_USER,
	S710_GET_WATCH,
	S710_GET_LOGO,
	S710_GET_BIKE,
	S710_GET_EXERCISE_1,
	S710_GET_EXERCISE_2,
	S710_GET_EXERCISE_3,
	S710_GET_EXERCISE_4,
	S710_GET_EXERCISE_5,
	S710_GET_REMINDER_1,
	S710_GET_REMINDER_2,
	S710_GET_REMINDER_3,
	S710_GET_REMINDER_4,
	S710_GET_REMINDER_5,
	S710_GET_REMINDER_6,
	S710_GET_REMINDER_7,
	S710_GET_FILES,
	S710_CONTINUE_TRANSFER,
	S710_CLOSE_CONNECTION,
	S710_SET_USER,
	S710_SET_WATCH,
	S710_SET_LOGO,
	S710_SET_BIKE,
	S710_SET_EXERCISE_1,
	S710_SET_EXERCISE_2,
	S710_SET_EXERCISE_3,
	S710_SET_EXERCISE_4,
	S710_SET_EXERCISE_5,
	S710_SET_REMINDER_1,
	S710_SET_REMINDER_2,
	S710_SET_REMINDER_3,
	S710_SET_REMINDER_4,
	S710_SET_REMINDER_5,
	S710_SET_REMINDER_6,
	S710_SET_REMINDER_7,
	S710_HARD_RESET
} S710_Packet_Index;

typedef enum {
	S710_UNITS_METRIC,
	S710_UNITS_ENGLISH
} S710_Units;

typedef enum {
	S710_HT_SHOW_LIMITS,
	S710_HT_STORE_LAP,
	S710_HT_SWITCH_DISP
} S710_Heart_Touch;

typedef enum {
	S710_RECORD_INT_05,
	S710_RECORD_INT_15,
	S710_RECORD_INT_60
} S710_Recording_Interval;

typedef enum {
	S710_INTERVAL_MODE_OFF = 0,
	S710_INTERVAL_MODE_ON  = 1
} S710_Interval_Mode;

typedef enum {
	S710_HRM_AUTO    =  0,
	S710_HRM_S610    = 11,  /* same as in hrm files */
	S710_HRM_S710    = 12,  /* same as in hrm files */
	S710_HRM_S810    = 13,  /* same as in hrm files */
	S710_HRM_S625X   = 22,  /* same as in hrm files */
	S710_HRM_UNKNOWN = 255
} S710_HRM_Type;

/* helpful macros */

#define UNIB(x)      ((x)>>4)
#define LNIB(x)      ((x)&0x0f)
#define BINU(x)      ((x)<<4)
#define BINL(x)      ((x)&0x0f)
#define BCD(x)       (UNIB(x)*10 + LNIB(x))
#define HEX(x)       ((((x)/10)<<4) + ((x)%10))
#define DIGITS01(x)  ((x)%100)
#define DIGITS23(x)  (((x)/100)%100)
#define DIGITS45(x)  (((x)/10000)%100)

#define CLAMP(a,b,c) if((a)<(b))(a)=(b);if((a)>(c))(a)=(c)

/* structure and type definitions */
struct s710_driver;

struct s710_driver_ops {
	int (*init)  (struct s710_driver* d);
	int (*read)  (struct s710_driver* d, unsigned char *byte);
	int (*write) (struct s710_driver* d, BUF *buf);
	int (*close) (struct s710_driver* d);
};

struct s710_driver {
	char                    path[PATH_MAX];
	void                   *data;
	struct s710_driver_ops *dops;
};

typedef char           S710_Label[8];

typedef struct S710_Time {
	int    hours;
	int    minutes;
	int    seconds;
	int    tenths;
} S710_Time;

/* basic types */

typedef unsigned char  S710_Heart_Rate;
typedef unsigned char  S710_Cadence;
typedef short          S710_Altitude;
typedef unsigned short S710_Speed;     /* 1/16ths of a km/hr */
typedef float          S710_Distance;

typedef struct S710_Power {
	unsigned short power;
	unsigned char  lr_balance;
	unsigned char  pedal_index;
} S710_Power;

typedef char           S710_Temperature;

typedef struct S710_HR_Limit {
	S710_Heart_Rate  lower;
	S710_Heart_Rate  upper;
} S710_HR_Limit;

/* basic communication packet */

typedef const char    *PacketName;
typedef unsigned char  PacketType;
typedef unsigned char  PacketID;
typedef unsigned short PacketLength;
typedef unsigned short PacketChecksum;
typedef unsigned char  PacketData[1];

typedef struct packet_t {
	PacketName     name;
	PacketType     type;
	PacketID       id;
	PacketLength   length;
	PacketChecksum checksum;
	PacketData     data;
} packet_t;

/* overview data */

typedef struct overview_t {
	int files;
	int bytes;
} overview_t;

/* logo data */

typedef struct logo_t {
	unsigned char column[47];
} logo_t;

/* exercise data */

typedef struct exercise_t {
	unsigned int            which;
	S710_Label              label;
	S710_Time               timer[3];
	S710_HR_Limit           hr_limit[3];
	S710_Time               recovery_time;
	S710_Heart_Rate         recovery_hr;
} exercise_t;

/* lap data */

typedef struct lap_data_t {
	S710_Time        split;
	S710_Time        cumulative;
	S710_Heart_Rate  lap_hr;
	S710_Heart_Rate  avg_hr;
	S710_Heart_Rate  max_hr;
	S710_Altitude    alt;
	S710_Altitude    ascent;
	S710_Altitude    cumul_ascent;
	S710_Temperature temp;
	S710_Cadence     cad;
	int              distance;
	int              cumul_distance;
	S710_Speed       speed;
	S710_Power       power;
} lap_data_t;


/* units info */

typedef struct units_data_t {
	S710_Units       system;       /* S710_UNITS_METRIC or S710_UNITS_ENGLISH */
	const char      *altitude;     /* "m" or "ft" */
	const char      *speed;        /* "kph" or "mph" */
	const char      *distance;     /* "km" or "mi" */
	const char      *temperature;  /* "C" or "F" */
} units_data_t;


/* a single workout */

typedef struct workout_t {
	S710_HRM_Type           type;
	struct tm               date;
	int                     ampm;
	time_t                  unixtime;
	int                     user_id;
	S710_Interval_Mode      interval_mode;
	int                     exercise_number;
	S710_Label              exercise_label;
	S710_Time               duration;
	S710_Heart_Rate         avg_hr;
	S710_Heart_Rate         max_hr;
	int                     bytes;
	int                     laps;
	int                     manual_laps;
	int                     samples;
	units_data_t            units;
	int                     mode;
	int                     recording_interval;
	int                     filtered;
	S710_HR_Limit           hr_limit[3];
	S710_Time               hr_zone[3][3];
	S710_Time               bestlap_split;
	S710_Time               cumulative_exercise;
	S710_Time               cumulative_ride;
	int                     odometer;
	int                     exercise_distance;
	S710_Cadence            avg_cad;
	S710_Cadence            max_cad;
	S710_Altitude           min_alt;
	S710_Altitude           avg_alt;
	S710_Altitude           max_alt;
	S710_Temperature        min_temp;
	S710_Temperature        avg_temp;
	S710_Temperature        max_temp;
	S710_Altitude           ascent;
	S710_Power              avg_power;
	S710_Power              max_power;
	S710_Speed              avg_speed;
	S710_Speed              max_speed;
	S710_Speed              median_speed;
	S710_Speed		  highest_speed;
	int                     energy;
	int                     total_energy;
	lap_data_t             *lap_data;
	S710_Heart_Rate        *hr_data;
	S710_Altitude          *alt_data;
	S710_Speed             *speed_data;
	S710_Distance          *dist_data;       /* computed from speed_data */
	S710_Cadence           *cad_data;
	S710_Power             *power_data;
} workout_t;


/* A few macros that are useful */

#define S710_HAS_FIELD(x,y)   (((x) & S710_MODE_##y) != 0)

#define S710_HAS_CADENCE(x)   S710_HAS_FIELD(x,CADENCE)
#define S710_HAS_POWER(x)     S710_HAS_FIELD(x,POWER)
#define S710_HAS_SPEED(x)     S710_HAS_FIELD(x,SPEED)
#define S710_HAS_ALTITUDE(x)  S710_HAS_FIELD(x,ALTITUDE)
#define S710_HAS_BIKE1(x)     S710_HAS_FIELD(x,BIKE1)

/* function prototypes */

/* driver.c */
int		 driver_init(const char *driver_name, const char *device);
int		 driver_open();
int		 driver_write(BUF *buf);
int		 driver_read_byte(unsigned char *b);
int		 driver_close();

/* serial.c */
int       ir_init(struct s710_driver *d);
int       ir_read_byte(struct s710_driver *d, unsigned char *byte);
int       ir_write(struct s710_driver *d, BUF *buf);
int       serial_init(struct s710_driver *d);
int       serial_read_byte(struct s710_driver *d, unsigned char *byte);
int       serial_write(struct s710_driver *d, BUF *buf);

/* usb.c */
int       usb_init_port(struct s710_driver *d);
int       usb_read_byte(struct s710_driver *d, unsigned char *byte);
int       usb_send_packet(struct s710_driver *d, BUF *buf);
int       usb_shutdown_port(struct s710_driver *d);

/* crc.c */
void crc_process ( unsigned short      * context,
				   unsigned char         ch );
void crc_block   ( unsigned short      * context,
				   const unsigned char * blk,
				   int                   len );

/* label.c */
void      label_extract(unsigned char *buf, S710_Label *label, int bytes);
void      label_encode(S710_Label label, unsigned char *buf, int bytes);

/* comm.c */
int       packet_send(packet_t *packet);
packet_t *packet_recv();

/* files.c */
int       files_get(BUF *files);
int       files_save(BUF *files, const char *dir);

/* packet.c */
packet_t *packet_get(S710_Packet_Index idx);
packet_t *packet_get_response(S710_Packet_Index request);
void      packet_print(packet_t *pkt, FILE *fp);

/* time.c */
void   		print_s710_time(S710_Time* t, const char *format, FILE *fp);
time_t 		s710_time_to_tenths(S710_Time *t);
time_t 		s710_time_to_seconds(S710_Time *t);
void   		diff_s710_time(S710_Time *t1, S710_Time *t2, S710_Time *diff);
void   		sum_s710_time(S710_Time *t1, S710_Time *t2, S710_Time *sum);
void   		increment_s710_time(S710_Time *t, unsigned int seconds);
float  		get_hours_from_s710_time(S710_Time *t);

/* workout_print.c */
void		workout_print(workout_t * w, FILE * fp, int what);

/* workout_read.c */
workout_t*	workout_read(char* filename, S710_HRM_Type type);

/* workout_util.c */
int  		workout_header_size(workout_t * w);
int  		workout_bytes_per_lap(S710_HRM_Type type, unsigned char bt, unsigned char bi);
int  		workout_bytes_per_sample(unsigned char bt);
int  		workout_allocate_sample_space(workout_t * w);
void 		workout_free(workout_t * w);

#endif /* __S710_H__ */
