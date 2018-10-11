#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "globals.h"
#include "memory.h"
#include "screen.h"
#include "memorymap.h"
#include "ascii.h"
#include "videomodes.h"

#define BORDER_COLOUR(X) POKE(0xD020U,X)
#define SCREEN_COLOUR(X) POKE(0xD021U,X)
#define SCREEN_COLOUR_OFFSET 0x17800U // COLOUR_RAM_ADDRESS - SCREEN_ADDRESS

#define MOD_LSHIFT      0x01
#define MOD_RSHIFT      0x02
#define MOD_SHIFT       0x03 // (MOD_LSHIFT | MOD_RSHIFT)
#define MOD_CTRL        0x04
#define MOD_SUPER       0x08
#define MOD_ALT         0x10

#define KEY_ESC         0x03
#define KEY_TAB         0x09
#define KEY_RETURN      0x0D
#define KEY_BACKSPACE   0x14

#define KEY_LEFT        0x9D
#define KEY_RIGHT       0x1D
#define KEY_LEFT_RIGHT  0x1D // (KEY_LEFT & KEY_RIGHT)
#define KEY_UP          0x91
#define KEY_DOWN        0x11
#define KEY_UP_DOWN     0x11 // (KEY_UP & KEY_DOWN)

// Screen ram attributes
#define SATTRIB_TRIM(x)     (x<<5)

// 8 bit colour ram attributes (high byte in 16bit mode)
#define CATTRIB_BLINK       0x10
#define CATTRIB_REVERSE     0x20
#define CATTRIB_BOLD        0x40
#define CATTRIB_UNDERLINE   0x80
#define CATTRIB_ALT_PALETTE (CATTRIB_BOLD | CATTRIB_REVERSE)

// 16 bit colour ram attributes
#define CATTRIB_TRIM_TOP    0x8
#define CATTRIB_GOTO        0x10
#define CATTRIB_ALPHA_BLEND 0x20
#define CATTRIB_HORI_FLIP   0x40
#define CATTRIB_VERT_FLIP   0x80

#define READ_KEY() (*(unsigned char *)0xD610)
#define READ_MOD() (*(unsigned char *)0xD611)

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