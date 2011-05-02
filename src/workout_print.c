#include <string.h>
#include "s710.h"

/*
   "what" is the bitwise or of at least one of:

   S710_WORKOUT_HEADER
   S710_WORKOUT_LAPS
   S710_WORKOUT_SAMPLES

   or it can just be S710_WORKOUT_FULL (everything)
*/

void
workout_print(workout_t * w, FILE * fp, int what)
{
	const char*  hrm_type = "Unknown";
	int          i;
	int          j;
	float        vam;
	char         buf[BUFSIZ];
	lap_data_t  *l;
	S710_Time    s;

	if (what & S710_WORKOUT_HEADER) {
		/* exercise date */
		strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S (%a, %d %b %Y)",
				 localtime(&w->unixtime));
		fprintf(fp,"Workout date:            %s\n",buf);

		/* HRM type */
		switch ( w->type ) {
		case S710_HRM_S610:
			hrm_type = "S610";
			break;
		case S710_HRM_S625X:
			hrm_type = "S625X";
			break;
		case S710_HRM_S710:
			hrm_type = "S710";
			break;
		default:
			break;
		}
		fprintf(fp,"HRM Type:                %s\n",hrm_type);

		/* user id */
		fprintf(fp,"User ID:                 %d\n",w->user_id);

		/* exercise number and label */
		if ( w->exercise_number > 0 && w->exercise_number <= 5 ) {
			fprintf(fp,"Exercise:                %d (%s)\n",
					w->exercise_number,
					w->exercise_label);
		}

		/* workout mode */
		fprintf(fp,"Mode:                    HR");
		if ( S710_HAS_ALTITUDE(w->mode) ) fprintf(fp,", Altitude");
		if ( S710_HAS_SPEED(w->mode) )
			fprintf(fp,", Bike %d (Speed%s%s)",
					S710_HAS_BIKE1(w->mode) ? 1 : 2,
					S710_HAS_POWER(w->mode) ? ", Power" : "",
					S710_HAS_CADENCE(w->mode) ? ", Cadence" : "");
		fprintf(fp,"\n");

		/* exercise duration */
		fprintf(fp,"Exercise duration:       ");
		print_s710_time(&w->duration,"hmst",fp);
		fprintf(fp,"\n");

		if ( S710_HAS_SPEED(w->mode) )
			fprintf(fp,"Exercise distance:       %.1f %s\n",
					w->exercise_distance/10.0, w->units.distance);

		/* recording interval */
		fprintf(fp,"Recording interval:      %d seconds\n",
				w->recording_interval);

		/* average, maximum heart rate */
		fprintf(fp,"Average heart rate:      %d bpm\n",w->avg_hr);
		fprintf(fp,"Maximum heart rate:      %d bpm\n",w->max_hr);

		/* average, maximum cadence */
		if ( S710_HAS_CADENCE(w->mode) ) {
			fprintf(fp,"Average cadence:         %d rpm\n",w->avg_cad);
			fprintf(fp,"Maximum cadence:         %d rpm\n",w->max_cad);
		}

		/* average, maximum speed */
		if ( S710_HAS_SPEED(w->mode) ) {
			fprintf(fp,"Average speed:           %.1f %s\n",
					w->avg_speed/16.0, w->units.speed);
			fprintf(fp,"Maximum speed:           %.1f %s\n",
					w->max_speed/16.0, w->units.speed);
		}

		if ( w->type != S710_HRM_S610 ) {
			/* min, avg, max temperature */
			fprintf(fp,"Minumum temperature:     %d %s\n",
					w->min_temp, w->units.temperature);
			fprintf(fp,"Average temperature:     %d %s\n",
					w->avg_temp, w->units.temperature);
			fprintf(fp,"Maximum temperature:     %d %s\n",
					w->max_temp, w->units.temperature);
		}

		/* altitude, ascent */
		if ( S710_HAS_ALTITUDE(w->mode) ) {
			fprintf(fp,"Minimum altitude:        %d %s\n",
					w->min_alt, w->units.altitude);
			fprintf(fp,"Average altitude:        %d %s\n",
					w->avg_alt, w->units.altitude);
			fprintf(fp,"Maximum altitude:        %d %s\n",
					w->max_alt, w->units.altitude);
			fprintf(fp,"Ascent:                  %d %s\n",
					w->ascent, w->units.altitude);
		}

		/* power data */
		if (  S710_HAS_POWER(w->mode)  ) {
			fprintf(fp,"Average power:           %d W\n",
					w->avg_power.power);
			fprintf(fp,"Average LR balance:      %d-%d\n",
					w->avg_power.lr_balance >> 1,
					100 - (w->avg_power.lr_balance >> 1));
			fprintf(fp,"Average pedal index:     %d %%\n",
					w->avg_power.pedal_index >> 1);
			fprintf(fp,"Maximum power:           %d W\n",
					w->max_power.power);
			fprintf(fp,"Maximum pedal index:     %d %%\n",
					w->max_power.pedal_index >> 1);
		}

		/* HR limits */
		for ( i = 0; i < 3; i++ ) {
			fprintf(fp,"HR Limit %d:              %d to %3d\n",
					i+1,w->hr_limit[i].lower,w->hr_limit[i].upper);
			fprintf(fp,"\tTime below:      ");
			print_s710_time(&w->hr_zone[i][0],"hms",fp);
			fprintf(fp,"\n");
			fprintf(fp,"\tTime within:     ");
			print_s710_time(&w->hr_zone[i][1],"hms",fp);
			fprintf(fp,"\n");
			fprintf(fp,"\tTime above:      ");
			print_s710_time(&w->hr_zone[i][2],"hms",fp);
			fprintf(fp,"\n");
		}

		/* energy, total energy (units??) */
		fprintf(fp,"Energy:                  %d\n",w->energy);
		fprintf(fp,"Total energy:            %d\n",w->total_energy);

		/* cumulative counters */
		fprintf(fp,"Cumulative exercise:     ");
		print_s710_time(&w->cumulative_exercise,"hm",fp);
		fprintf(fp,"\n");

		if ( S710_HAS_SPEED(w->mode) ) {
			fprintf(fp,"Cumulative ride time:    ");
			print_s710_time(&w->cumulative_ride,"hm",fp);
			fprintf(fp,"\n");
			fprintf(fp,"Odometer:                %d %s\n",
					w->odometer, w->units.distance);
		}

		/* laps */
		fprintf(fp,"Laps:                    %d\n",w->laps);
		fprintf(fp,"\n\n");
	}

	if ( what & S710_WORKOUT_LAPS ) {
		/* lap data */
		for ( i = 0; i < w->laps; i++ ) {
			l = &w->lap_data[i];
			fprintf(fp,"Lap %d:\n",i+1);
			fprintf(fp,"\tLap split:          ");
			print_s710_time(&l->split,"hmst",fp);
			fprintf(fp,"\n");
			fprintf(fp,"\tLap cumulative:     ");
			print_s710_time(&l->cumulative,"hmst",fp);
			fprintf(fp,"\n");
			fprintf(fp,"\tLap HR:             %d bpm\n",l->lap_hr);
			fprintf(fp,"\tAverage HR:         %d bpm\n",l->avg_hr);
			fprintf(fp,"\tMaximum HR:         %d bpm\n",l->max_hr);

			if ( S710_HAS_ALTITUDE(w->mode) ) {
				fprintf(fp,"\tLap altitude:       %d %s\n",
						l->alt,w->units.altitude);
				fprintf(fp,"\tLap ascent:         %d %s\n",
						l->ascent,w->units.altitude);
				fprintf(fp,"\tLap cumulat. asc:   %d %s\n",
						l->cumul_ascent,w->units.altitude);
				fprintf(fp,"\tLap temperature:    %d %s\n",
						l->temp,w->units.temperature);
			}

			if ( S710_HAS_CADENCE(w->mode) )
				fprintf(fp,"\tLap cadence:        %d rpm\n",l->cad);

			if ( S710_HAS_SPEED(w->mode) ) {
				float  lap_speed = 0;
				time_t tenths;

				fprintf(fp,"\tLap distance:       %.1f %s\n",
						l->distance/10.0, w->units.distance);
				fprintf(fp,"\tLap cumulat. dist:  %.1f %s\n",
						l->cumul_distance/10.0, w->units.distance);
				fprintf(fp,"\tLap speed at end:   %.1f %s\n",
						l->speed/16.0, w->units.speed);
				tenths = s710_time_to_tenths(&l->split);
				if ( tenths > 0 ) {
					/* note the cancelling factors of 10 */
					lap_speed = l->distance / ((float)tenths/3600.0);
				}
				fprintf(fp,"\tLap speed ave:      %.1f %s\n",
						lap_speed, w->units.speed);
			}

			if ( S710_HAS_POWER(w->mode) ) {
				fprintf(fp,"\tLap power:          %d W\n",
						l->power.power);
				fprintf(fp,"\tLap LR balance:     %d-%d\n",
						l->power.lr_balance >> 1,
						100 - (l->power.lr_balance >> 1));
				fprintf(fp,"\tLap pedal index:    %d%%\n",
						l->power.pedal_index >> 1);
			}

			fprintf(fp,"\n");
		}
	}

	if ( what & S710_WORKOUT_SAMPLES ) {
		/* sample data */
		fprintf(fp,"\nRecorded data:\n\n");
		fprintf(fp,"Time\t\t HR");

		if ( S710_HAS_ALTITUDE(w->mode) ) {
			fprintf(fp,"\t Alt\t    VAM");
		}

		if ( S710_HAS_SPEED(w->mode) ) {
			fprintf(fp,"\t  Spd");
			if ( S710_HAS_POWER(w->mode) ) { fprintf(fp,"\tPower  LR Bal   PI"); }
			if ( S710_HAS_CADENCE(w->mode) ) { fprintf(fp,"\tCad"); }
			fprintf(fp,"\t  Dist");
		}
		fprintf(fp,"\n");

		memset(&s,0,sizeof(s));
		for ( i = 0; i < w->samples; i++ ) {
			print_s710_time(&s,"hms",fp);
			fprintf(fp,"\t\t%3d",w->hr_data[i]);

			if ( S710_HAS_ALTITUDE(w->mode) ) {
				/* compute VAM as the average of the past 60 seconds... */
				j = (i >= 60/w->recording_interval) ? i-60/w->recording_interval : 0;
				if ( i > j ) {
					vam = (float)(w->alt_data[i] - w->alt_data[j]) * 3600.0 /
						((i-j) * w->recording_interval);
				} else {
					vam = 0.0;
				}
				fprintf(fp,"\t%4d\t%7.1f",w->alt_data[i],vam);
			}

			if ( S710_HAS_SPEED(w->mode) ) {
				fprintf(fp,"\t%5.1f",w->speed_data[i]/16.0);
				if ( S710_HAS_POWER(w->mode) ) { fprintf(fp,"\t%5d\t%2d-%2d\t%2d",
														 w->power_data[i].power,
														 w->power_data[i].lr_balance >> 1,
														 100 - (w->power_data[i].lr_balance >> 1),
														 w->power_data[i].pedal_index >> 1);
				}

				if ( S710_HAS_CADENCE(w->mode) ) {
					fprintf(fp,"\t%3d",w->cad_data[i]);
				}
				fprintf(fp,"\t%6.2f",w->dist_data[i]);
			}
			fprintf(fp,"\n");

			increment_s710_time(&s,w->recording_interval);
		}
	}
}
