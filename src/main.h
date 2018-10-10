#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "memory.h"
#include "screen.h"
#include "pointermap.h"
#include "ascii.h"

#define BORDER_COLOUR(X) POKE(0xD020U,X)
#define SCREEN_COLOUR(X) POKE(0xD021U,X)
#define SCREEN_SMALL_COLOUR_OFFSET 0x17800U // SMALL_SMALL_COLOUR_RAM_ADDRESS - SCREEN_ADDRESS

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

#define READ_KEY() (*(unsigned char *)0xD610)
#define READ_MOD() (*(unsigned char *)0xD611)

#define SCROLL_DOWN() {\
    long _count = 23 * 80; \
    lcopy(SCREEN_ADDRESS + 80, SCREEN_ADDRESS, _count); \
    lcopy(SMALL_COLOUR_RAM_ADDRESS + 80, SMALL_COLOUR_RAM_ADDRESS, _count); \
    lfill(SCREEN_ADDRESS + _count, ' ', 80); \
    lfill(SMALL_COLOUR_RAM_ADDRESS + _count, 1, 80); \
    screen_line_address -= 80; \
}

#define SCROLL_UP() {\
    long _index = 23 * 80; \
    for (; _index > 0; _index -= 80) { \
        lcopy(SCREEN_ADDRESS + _index - 80, SCREEN_ADDRESS + _index, 80); \
        lcopy(SMALL_COLOUR_RAM_ADDRESS + _index - 80, SMALL_COLOUR_RAM_ADDRESS + _index, 80); \
    } \
    lfill(SCREEN_ADDRESS, ' ', 80); \
    lfill(SMALL_COLOUR_RAM_ADDRESS, 1, 80); \
    screen_line_address += 80; \
}

#define SET_CURSOR_ATTRIB(x) \
    lpoke(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET, (lpeek(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET) & 0xF) | x)
#define SET_CURSOR(x) { \
    char _cursor = lpeek(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET); \
    lpoke(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET, _cursor & 0xF); \
    screen_line_address = x + SCREEN_ADDRESS; \
    if (screen_line_address < SCREEN_ADDRESS) SCROLL_UP() \
    else if (screen_line_address - SCREEN_ADDRESS > 24*80) SCROLL_DOWN() \
    lpoke(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET, \
        (lpeek(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET) & 0xF) | _cursor & 0xF0); \
}
#define MOVE_CURSOR(x) { \
    char _cursor = lpeek(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET); \
    lpoke(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET, _cursor & 0xF); \
    screen_line_address += x; \
    if (screen_line_address < SCREEN_ADDRESS) SCROLL_UP() \
    else if (screen_line_address - SCREEN_ADDRESS > 24*80) SCROLL_DOWN() \
    lpoke(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET, \
        (lpeek(screen_line_address+SCREEN_SMALL_COLOUR_OFFSET) & 0xF) | _cursor & 0xF0); \
}
#define WRITE_STRING(s) { \
    char _strlen = 0; \
    while (s[_strlen]) ++_strlen; \
    if((screen_line_address + _strlen) - SCREEN_ADDRESS >= 24*80) \
        SCROLL_DOWN(); \
    lcopy((long)&s[0], screen_line_address, _strlen); \
    MOVE_CURSOR(_strlen); \
}
#define WRITE_CHAR(c) { \
    if((screen_line_address + 1) - SCREEN_ADDRESS >= 24*80) \
        SCROLL_DOWN(); \
    POKE(screen_line_address, c); \
    MOVE_CURSOR(1); \
}