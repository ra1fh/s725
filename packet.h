/* packet.h - sending/receiving of packets */

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

#ifndef PACKET_H
#define PACKET_H

#include <sys/types.h>

typedef struct packet packet_t;

#define S725_REQUEST         0xa3
#define S725_RESPONSE        0x5c

typedef enum {
	S725_PACKET_INDEX_INVALID = -1,
	S725_GET_OVERVIEW = 0,
	S725_GET_USER,
	S725_GET_WATCH,
	S725_GET_LOGO,
	S725_GET_BIKE,
	S725_GET_EXERCISE_1,
	S725_GET_EXERCISE_2,
	S725_GET_EXERCISE_3,
	S725_GET_EXERCISE_4,
	S725_GET_EXERCISE_5,
	S725_GET_REMINDER_1,
	S725_GET_REMINDER_2,
	S725_GET_REMINDER_3,
	S725_GET_REMINDER_4,
	S725_GET_REMINDER_5,
	S725_GET_REMINDER_6,
	S725_GET_REMINDER_7,
	S725_GET_FILES,
	S725_CONTINUE_TRANSFER,
	S725_CLOSE_CONNECTION,
	S725_SET_USER,
	S725_SET_WATCH,
	S725_SET_LOGO,
	S725_SET_BIKE,
	S725_SET_EXERCISE_1,
	S725_SET_EXERCISE_2,
	S725_SET_EXERCISE_3,
	S725_SET_EXERCISE_4,
	S725_SET_EXERCISE_5,
	S725_SET_REMINDER_1,
	S725_SET_REMINDER_2,
	S725_SET_REMINDER_3,
	S725_SET_REMINDER_4,
	S725_SET_REMINDER_5,
	S725_SET_REMINDER_6,
	S725_SET_REMINDER_7,
	S725_HARD_RESET,
	S725_LISTEN = 255,
} S725_Packet_Index;

int       packet_send(packet_t *packet);
packet_t *packet_recv(void);
packet_t *packet_get(S725_Packet_Index idx);
packet_t *packet_get_response(S725_Packet_Index request);
packet_t *packet_listen(void);
u_char   *packet_data(packet_t *p);
u_short   packet_len(packet_t *p);
u_char    packet_get_type(packet_t *p);

#endif	/* PACKET_H */
