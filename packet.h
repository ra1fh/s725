/*
 * sending/receiving of packets
 */

#ifndef PACKET_H
#define PACKET_H

#include <sys/types.h>

typedef struct packet packet_t;

#define S710_REQUEST         0xa3
#define S710_RESPONSE        0x5c

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

int       packet_send(packet_t *packet);
packet_t *packet_recv();
packet_t *packet_get(S710_Packet_Index idx);
packet_t *packet_get_response(S710_Packet_Index request);
void      packet_print(packet_t *pkt, FILE *fp);
u_char   *packet_data(packet_t *p);
u_short   packet_len(packet_t *p);

#endif	/* PACKET_H */
