#include "megaint.h"
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
#define ATTRIB_4BIT        0x08
#define ATTRIB_GOTO        0x10
#define ATTRIB_ALPHA_BLEND 0x20
#define ATTRIB_HORI_FLIP   0x40
#define ATTRIB_VERT_FLIP   0x80

#define SCREEN_ADDRESS (0x8000U)
#define CHARSET_ADDRESS (0x8800U)
#define COLOUR_RAM_ADDRESS (0x1F800U)
#define FOOTER_ADDRESS (SCREEN_ADDRESS+24*80)

#define FOOTER_COPYRIGHT     0
#define FOOTER_BLANK         1
#define FOOTER_FATAL         2
#define FOOTER_MAX           2

#define ATTRIB_REVERSE 0x20
#define ATTRIB_BLINK 0x10
#define ATTRIB_UNDERLINE 0x80
#define ATTRIB_HIGHLIGHT 0x40

#define COLOUR_BLACK        0x0
#define COLOUR_WHITE        0x1
#define COLOUR_RED          0x2
#define COLOUR_CYAN         0x3
#define COLOUR_PURPLE       0x4
#define COLOUR_GREEN        0x5
#define COLOUR_BLUE         0x6
#define COLOUR_YELLOW       0x7
#define COLOUR_ORANGE       0x8
#define COLOUR_BROWN        0x9
#define COLOUR_PINK         0xA
#define COLOUR_GREY1        0xB
#define COLOUR_DARKGREY     0xB
#define COLOUR_GREY2        0xC
#define COLOUR_GREY         0xC
#define COLOUR_MEDIUMGREY   0xC
#define COLOUR_LIGHTGREEN   0xD
#define COLOUR_LIGHTBLUE    0xE
#define COLOUR_GREY3        0xF
#define COLOUR_LIGHTGREY    0xF

#define READ_KEY() (*(charptr_t)0xD610)
#define READ_MOD() (*(charptr_t)0xD611)

#define TOGGLE_BACK() POKE(0xD020U, (PEEK(0xD020U) + 1) & 0xF)
#define BLANK_SCREEN() POKE(0xD011U, PEEK(0xD011U) & ~0x10)
#define UNBLANK_SCREEN() POKE(0xD011U, PEEK(0xD011U) | 0x10)
#define TOGGLE_BLANK_SCREEN() POKE(0xD011U, PEEK(0xD011U) ^ 0x10)

#define DISABLE_HOT_REGISTERS() POKE(0xD05DU, PEEK(0xD05DU) & 0x7F)
#define ENABLE_HOT_REGISTERS() POKE(0xD05DU, PEEK(0xD05DU) | 0x80)

extern int32_t cursor_position;
extern uint8_t cursor_attrib[2];

extern int8_t char_size;
extern uint8_t char_attrib[2];

extern uint32_t screen_width;
extern uint32_t screen_size;

void toggle_write_protection(void);
