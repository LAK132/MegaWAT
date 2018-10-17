#include <stdint.h>
#include "memorymap.h"

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
#define ATTRIB_TRIM_RIGHT(x) (x<<5)

// 8 bit colour ram attributes (high byte in 16bit mode)
#define ATTRIB_BLINK       0x10
#define ATTRIB_REVERSE     0x20
#define ATTRIB_BOLD        0x40
#define ATTRIB_UNDERLINE   0x80
#define ATTRIB_ALT_PALETTE (ATTRIB_BOLD | ATTRIB_REVERSE)

// 16 bit colour ram attributes
#define ATTRIB_TRIM_TOP    0x8
#define ATTRIB_GOTO        0x10
#define ATTRIB_ALPHA_BLEND 0x20
#define ATTRIB_HORI_FLIP   0x40
#define ATTRIB_VERT_FLIP   0x80

#define READ_KEY() (*(unsigned char *)0xD610)
#define READ_MOD() (*(unsigned char *)0xD611)

extern int32_t cursor_position;
extern uint8_t cursor_attrib[2];

extern int8_t char_size;
extern uint8_t char_attrib[2];

extern uint32_t screen_address;
extern uint32_t colour_address;
extern uint32_t screen_width;
extern uint32_t screen_size;

extern uint8_t c, x, y, z;
extern uint32_t i, j, k, l;

void toggle_write_protection(void);

// extern uint32_t full_screen_buffer;
// extern uint32_t full_screen_buffer_size;
// extern uint32_t full_colour_buffer;
// extern uint32_t full_colour_buffer_size;

// extern uint32_t small_screen_buffer;
// extern uint32_t small_screen_buffer_size;
// extern uint32_t small_colour_buffer;
// extern uint32_t small_colour_buffer_size;