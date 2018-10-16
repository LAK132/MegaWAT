#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "globals.h"
#include "f65.h"
#include "memory.h"
#include "screen.h"
#include "memorymap.h"
#include "ascii.h"
#include "videomodes.h"

void scrollDown(void);
void scrollUp(void);
// void liftCursor(void);
void applyAttrib(int32_t len);
#define liftCursor() applyAttrib(1);
void setCursor(int32_t x);
#define moveCursor(x) setCursor(((cursor_position/char_size) + x))
void writeString(void *str, uint32_t len);
void writeChar(uint16_t c);
void writeChars(uint16_t c, int32_t len);
uint32_t wstrlen(uint16_t *str);


void editor_show_cursor(void);
void editor_poll_keyboard(void);

