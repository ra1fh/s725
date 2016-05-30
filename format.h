/* format.h - output format helper functions */

/*
 * Copyright (C) 2016  Ralf Horstmann
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

#ifndef FORMAT_H
#define FORMAT_H

enum {
	FORMAT_HRM,
	FORMAT_RAW,
	FORMAT_TCX,
	FORMAT_TXT,
	FORMAT_UNKNOWN
};

int format_from_str(const char *format);
const char* format_to_str(int format);

#endif	/* FORMAT_H */
