#include <stdio.h>
#include <string.h>

#include "megaint.h"

#include "globals.h"
#include "f65.h"
#include "memory.h"
#include "screen.h"
#include "videomodes.h"
#include "editor.h"
#include "serial.h"

#ifdef __MEGA65__
#include "ascii.h"
#endif

extern int16_t font_file;
extern int16_t font_file_size;

#ifndef __MEGA65__
void megamain();
#endif