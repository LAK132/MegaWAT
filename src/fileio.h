#include "globals.h"
#include "memory.h"
#include "videomodes.h"
#include "errno.h"
#include "cbm.h"

#ifndef FILEIO_H
#define FILEIO_H

#include "editor.h"

#define FILE_NAME_MAX_LEN 16
extern uint8_t file_name[FILE_NAME_MAX_LEN];

// loads a presentation from disk
int fileio_load_pres(void);
// opens a file from disk to save the current presentation
int fileio_save_pres(void);
// load a font from the disk
int fileio_load_font(void);

#endif // FILEIO_H