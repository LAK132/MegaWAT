#include "globals.h"
#include "memory.h"
#include "videomodes.h"
#include "errno.h"
#include "cbm.h"

#ifndef FILE_NAME_MAX_LEN
#define FILE_NAME_MAX_LEN 16
#endif

#ifndef FILEIO_H
#define FILEIO_H

#include "editor.h"

extern uint8_t file_name[FILE_NAME_MAX_LEN];
extern uint8_t data_buffer[128];

// loads a presentation from disk
int fileio_load_pres(void);
// opens a file from disk to save the current presentation
int fileio_save_pres(void);
// load a font from the disk
int fileio_load_font(void);

#endif // FILEIO_H