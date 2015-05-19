/*
 * high level functions for receiving and saving files
 */

#ifndef FILES_H
#define FILES_H

#include "buf.h"

int files_get(BUF *files);
int files_split(BUF *files, int *offset, BUF *out);
time_t files_timestamp(BUF *f, size_t offset);

#endif	/* FILES_H */
