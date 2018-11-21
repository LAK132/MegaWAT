#include "globals.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"

#ifndef FILEIO_H
#define FILEIO_H

#define FILE_NAME_MAX_LEN 16
extern uint8_t file_name[FILE_NAME_MAX_LEN];

// creates a new file on disk
int fileio_create_pres(void);
// loads a presentation from disk
int fileio_load_pres(void);
// opens a file from disk to save the current presentation
int fileio_save_pres(void);

#endif // FILEIO_H