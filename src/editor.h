#include "f65.h"
#include "globals.h"
#include "memory.h"
#include "screen.h"
#include "videomodes.h"
#include "serial.h"
#include "stdio.h"
#include "string.h"
#include "fileio.h"

#ifndef EDITOR_H
#define EDITOR_H

#define EDITOR_END_SLIDE 16
#define EDITOR_MAX_SLIDES (EDITOR_END_SLIDE + 1)
extern ptr_t slide_start[EDITOR_MAX_SLIDES];
extern uint8_t slide_colour[EDITOR_MAX_SLIDES];

void editor_show_cursor(void);
void editor_poll_keyboard(void);
void editor_initialise(void);

#endif // EDITOR_H