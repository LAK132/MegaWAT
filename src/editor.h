#include "f65.h"
#include "globals.h"
#include "memory.h"
#include "videomodes.h"
#include "serial.h"
#include "stdio.h"
#include "string.h"
#include "megastring.h"
#include "fileio.h"

#ifndef EDITOR_H
#define EDITOR_H

// XXX For now, have a fixed 128x30 buffer for text
// (We allow 128 instead of 100, so that we can have a number of font and attribute/colour changes
// in there.)
#define EDITOR_LINE_LEN 128

#define EDITOR_END_LINE 60
#define EDITOR_MAX_LINES (EDITOR_END_LINE + 1)

#define EDITOR_END_SLIDE 32
#define EDITOR_MAX_SLIDES (EDITOR_END_SLIDE + 1)

extern string_t presentation;
extern uint16_t slide_start[EDITOR_MAX_SLIDES];
extern uint8_t slide_colour[EDITOR_MAX_SLIDES];
extern uint8_t slide_resolution[EDITOR_MAX_SLIDES];
extern uint8_t slide_font_pack[EDITOR_MAX_SLIDES][FILE_NAME_MAX_LEN-6];

void editor_show_cursor(void);
void editor_poll_keyboard(void);
void editor_start(void);
void editor_show_message(uint8_t line, uint8_t *str);

#endif // EDITOR_H