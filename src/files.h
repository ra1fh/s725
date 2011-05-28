/*
 * high level functions for receiving and saving files
 */

#ifndef FILES_H
#define FILES_H

#include "buf.h"

int files_get(BUF *files);
int files_save(BUF *files, const char *dir);

#endif	/* FILES_H */
