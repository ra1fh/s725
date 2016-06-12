/* files.h - high level functions for receiving and saving files */

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

#ifndef FILES_H
#define FILES_H

#include "buf.h"

int files_get(BUF *files);
int files_listen(BUF *files);
int files_split(BUF *files, int *offset, BUF *out);
time_t files_timestamp(BUF *f, size_t offset);

#endif	/* FILES_H */
