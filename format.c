/* format.c - output format helper functions */

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

#include <string.h>

#include "format.h"

int
format_from_str(const char *format)
{
	if (format == NULL) {
		return FORMAT_UNKNOWN;
	} else if (!strcmp(format, "hrm")) {
		return FORMAT_HRM;
	} else if (!strcmp(format, "srd")) {
		return FORMAT_SRD;
	} else if (!strcmp(format, "tcx")) {
		return FORMAT_TCX;
	} else if (!strcmp(format, "txt")) {
		return FORMAT_TXT;
	}
	return FORMAT_UNKNOWN;
}

const char*
format_to_str(int format)
{
	switch(format) {
	case FORMAT_SRD:
		return "srd";
		break;
	case FORMAT_TCX:
		return "tcx";
		break;
	case FORMAT_TXT:
		return "txt";
		break;
	case FORMAT_HRM:
		return "hrm";
		break;
	}
	return "unknown";
}

