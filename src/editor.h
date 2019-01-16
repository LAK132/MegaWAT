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

#define EDITOR_END_SLIDE 16
#define EDITOR_MAX_SLIDES (EDITOR_END_SLIDE + 1)

// line functions
uint8_t editor_line_length(line_t *line);
#define editor_line_end(lineptr) (lineptr + editor_line_length(lineptr) + 1)
void editor_line_trim(line_t *line, uint8_t length);
// copy a line from one memory space to another
void editor_copy_line(line_t *restrict src, line_t *restrict dst);
// // shift characters after <position> to the right by <count> to allow characters to be inserted before <position>
// void editor_line_insert_space(line_t *line, uint8_t position, uint8_t count);
// // shift characters from <position + count> to the left by <count> to remove characters between <position> and <position + count>
// void editor_line_remove_space(line_t *line, uint8_t position, uint8_t count);

// slide functions
line_t editor_slide_get_line(slide_t *slide, uint8_t number);
line_t editor_next_line(line_t line);
void editor_slide_insert_line_space(slide_t *slide, uint8_t number, uint8_t position, uint8_t count);
void editor_slide_remove_line_space(slide_t *slide, uint8_t number, uint8_t position, uint8_t count);
// copy a slide from one memory space to another
typedef enum { ASIS, COMPRESS, DECOMPRESS } editor_copy_mode_t;
void editor_copy_slide(slide_t *restrict src, slide_t *restrict dst, editor_copy_mode_t mode);
// compress slide at <src> by overlapping lines that don't fill up the full width of the screen (still null terminated)
// stores compressed slide at <dst>
// void editor_compress_slide(slide_t *restrict src, slide_t *restrict dst);
// takes a compressed slide at <src> and decompresses is for editing, storing the decompressed slide at <dst>
// void editor_decompress_slide(slide_t *restrict src, slide_t *restrict dst);

// only valid for decompressed slides
void editor_slide_insert_lines(slide_t *slide, uint8_t position, uint8_t count);
void editor_slide_remove_lines(slide_t *slide, uint8_t position, uint8_t count);

// presentation functions
void editor_load_pres(presentation_t *pres, const char *str);
void editor_save_pres(presentation_t *pres, const char *str);
void editor_pres_insert_slides(presentation_t *pres, uint8_t position, uint8_t count);
void editor_pres_remove_slides(presentation_t *pres, uint8_t position, uint8_t count);

extern ptr_t slide_start[EDITOR_MAX_SLIDES];
extern uint8_t slide_colour[EDITOR_MAX_SLIDES];
extern uint8_t slide_resolution[EDITOR_MAX_SLIDES];
extern uint8_t slide_font_pack[EDITOR_MAX_SLIDES][FILE_NAME_MAX_LEN-6];

void editor_show_cursor(void);
void editor_poll_keyboard(void);
void editor_start(void);
void editor_show_message(uint8_t line, uint8_t *str);

#endif // EDITOR_H