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

#include "megaint.h"

// void setup_screen(void);

void display_footer(uint8_t index);
void footer_save(void);
void footer_restore(void);
void display_buffer_position_footer(int8_t bid);

void screen_colour_line(uint8_t line, uint8_t colour);
#define screen_colour_line_segment(LA,W,C) lfill(LA+(0x1f800-SCREEN_ADDRESS),C,W)

void screen_hex(shortptr_t addr, int32_t value);
void screen_hex_byte(shortptr_t addr, int32_t value);
void screen_decimal(shortptr_t addr, uint16_t value);
void set_screen_attributes(ptr_t p, uint8_t count, uint8_t attr);
// void write_line(char *s,char col);
// void recolour_last_line(char colour);
// char read_line(char *buffer, unsigned char maxlen);

void format_decimal(const shortptr_t addr, const int16_t value, const int8_t columns);
void format_hex(const shortptr_t addr, const int32_t value, const int8_t columns);

// extern long screen_line_address;

extern uint8_t ascii_map[256];
#define ascii_to_screen(X) ascii_map[X]

void fatal_error(charptr_t filename, uint16_t line_number);
#define FATAL_ERROR fatal_error(__FILE__,__LINE__)
