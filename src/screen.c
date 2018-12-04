#include "screen.h"
#include "memory.h"
#include "main.h"

#ifdef __MEGA65__
#include "ascii.h"
#endif

extern charptr_t charset;

// long screen_line_address = SCREEN_ADDRESS;
int8_t screen_column = 0;

charptr_t footer_messages[FOOTER_MAX + 1] = {
    "MEGA WAT! : (C) COPYRIGHT 2017-2018 PAUL GARDNER-STEPHEN ETC.    LAK132 was here",
    "                                                                                ",
    "A FATAL ERROR HAS OCCURRED, SORRY.                                              "
};

uint8_t screen_hex_buffer[6];

uint8_t screen_hex_digits[16] = {
        '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 0x41, 0x42, 0x43, 0x44, 0x45, 0x46
};

uint8_t to_screen_hex(uint8_t c)
{
    return screen_hex_digits[c & 0xf];
}

void screen_hex_byte(uint16_t addr, int32_t value)
{
    POKE(addr + 0, to_screen_hex(value >> 4));
    POKE(addr + 1, to_screen_hex(value >> 0));
}

void screen_hex(shortptr_t addr, int32_t value)
{
    POKE(addr + 0, to_screen_hex(value >> 28));
    POKE(addr + 1, to_screen_hex(value >> 24));
    POKE(addr + 2, to_screen_hex(value >> 20));
    POKE(addr + 3, to_screen_hex(value >> 16));
    POKE(addr + 4, to_screen_hex(value >> 12));
    POKE(addr + 5, to_screen_hex(value >> 8));
    POKE(addr + 6, to_screen_hex(value >> 4));
    POKE(addr + 7, to_screen_hex(value >> 0));
}

int8_t dec9[9];
void format_hex(const shortptr_t addr, const int32_t value, const int8_t columns)
{
    static uint8_t c;
    static uint32_t i;

    screen_hex((shortptr_t)&dec9[0], value);

    c = 8 - columns;
    while (c)
    {
        for (i = 0; i < 7; ++i)
            dec9[i] = dec9[i + 1];
        dec9[7] = ' ';
        c--;
    }
    for (i = 0; i < columns; ++i)
        lpoke(addr + i, dec9[i]);
}

uint8_t screen_decimal_digits[16][5] = {
    {0, 0, 0, 0, 1},
    {0, 0, 0, 0, 2},
    {0, 0, 0, 0, 4},
    {0, 0, 0, 0, 8},
    {0, 0, 0, 1, 6},
    {0, 0, 0, 3, 2},
    {0, 0, 0, 6, 4},
    {0, 0, 1, 2, 8},
    {0, 0, 2, 5, 6},
    {0, 0, 5, 1, 2},
    {0, 1, 0, 2, 4},
    {0, 2, 0, 4, 8},
    {0, 4, 0, 9, 6},
    {0, 8, 1, 9, 2},
    {1, 6, 3, 8, 4},
    {3, 2, 7, 6, 8}
};

// void write_line(char *s, char col)
// {
//     char len = 0;
//     while (s[len])
//         len++;
//     lcopy((long)&s[0], screen_line_address + col, len);
//     screen_line_address += 80;
//     if ((screen_line_address - SCREEN_ADDRESS) >= (24 * 80))
//     {
//         screen_line_address -= 80;
//         lcopy(SCREEN_ADDRESS + 80, SCREEN_ADDRESS, 23 * 80);
//         lcopy(COLOUR_RAM_ADDRESS + 80, COLOUR_RAM_ADDRESS, 23 * 80);
//         lfill(SCREEN_ADDRESS + 23 * 80, ' ', 80);
//         lfill(COLOUR_RAM_ADDRESS + 23 * 80, 1, 80);
//     }
// }

// void recolour_last_line(char colour)
// {
//     long colour_address = COLOUR_RAM_ADDRESS + (screen_line_address - SCREEN_ADDRESS) - 80;
//     lfill(colour_address, colour, 80);
//     return;
// }

uint8_t carry, temp;
uint16_t value;
void screen_decimal(shortptr_t addr, uint16_t v)
{
    static uint32_t j, k;
    // XXX - We should do this off-screen and copy into place later, to avoid glitching
    // on display.

    value = v;

    // Start with all zeros
    for (j = 0; j < 5; ++j)
        screen_hex_buffer[j] = 0;

    // Add power of two strings for all non-zero bits in value.
    // XXX - We should use BCD mode to do this more efficiently
    for (j = 0; j < 16; ++j)
    {
        if (value & 1)
        {
            carry = 0;
            for (k = 4; k < 128; --k)
            {
                temp = screen_hex_buffer[j] + screen_decimal_digits[j][k] + carry;
                if (temp > 9)
                {
                    temp -= 10;
                    carry = 1;
                }
                else
                    carry = 0;
                screen_hex_buffer[k] = temp;
            }
        }
        value = value >> 1;
    }

    // Now convert to ascii digits
    for (k = 0; k < 5; ++k)
        screen_hex_buffer[k] = screen_hex_buffer[k] | '0';

    // and shift out leading zeros
    for (k = 0; k < 4; ++k)
    {
        if (screen_hex_buffer[0] != '0')
            break;
        screen_hex_buffer[0] = screen_hex_buffer[1];
        screen_hex_buffer[1] = screen_hex_buffer[2];
        screen_hex_buffer[2] = screen_hex_buffer[3];
        screen_hex_buffer[3] = screen_hex_buffer[4];
        screen_hex_buffer[4] = ' ';
    }

    // Copy to screen
    for (k = 0; k < 5; ++k)
        POKE(addr + k, screen_hex_buffer[k]);
}

int8_t dec6[6];
void format_decimal(const shortptr_t addr, const int16_t value, const int8_t columns)
{
    static uint32_t i;

    screen_decimal((shortptr_t)&dec6[0], value);

    for (i = 0; i < columns; ++i)
        lpoke(addr + i, dec6[i]);
}

ptr_t addr;
void display_footer(uint8_t index)
{
    addr = (ptr_t)footer_messages[index];
    lcopy(addr, FOOTER_ADDRESS, 80);
    set_screen_attributes(FOOTER_ADDRESS, 80, ATTRIB_REVERSE);
}

// void setup_screen(void)
// {
//     uint8_t v;

//     m65_io_enable();

//     // 80-column mode, fast CPU, extended attributes enable
//     *((charptr_t)0xD031) = 0xe0;

//     // Put screen memory somewhere (2KB required)
//     // We are using $8000-$87FF for screen
//     // Using custom charset @ $A000
//     *(charptr_t)0xD018U =
//             (((CHARSET_ADDRESS - 0x8000U) >> 11) << 1) + (((SCREEN_ADDRESS - 0x8000U) >> 10) << 4);

//     // VIC RAM Bank to $8000-$BFFF
//     v = *(charptr_t)0xDD00U;
//     v &= 0xfc;
//     v |= 0x01;
//     *(charptr_t)0xDD00U = v;

//     // Screen colours
//     POKE(0xD020U, 0);
//     POKE(0xD021U, 6);

//     // Clear screen RAM
//     lfill(SCREEN_ADDRESS, 0x20, 2000);

//     // Clear colour RAM: white text
//     lfill(0x1f800, 0x01, 2000);

//     // Copy ASCII charset into place
//     lcopy((int)&charset[0], CHARSET_ADDRESS, 0x800);

//     // Set screen line address and write point
//     screen_line_address = SCREEN_ADDRESS;
//     screen_column = 0;

//     display_footer(FOOTER_COPYRIGHT);
// }

void screen_colour_line(uint8_t line, uint8_t colour)
{
    // Set colour RAM for this screen line to this colour
    // (use bit-shifting as fast alternative to multiply)
    lfill(0x1f800 + (line << 6) + (line << 4), colour, 80);
}

void fatal_error(charptr_t filename, uint16_t line_number)
{
    static uint32_t i;

    display_footer(FOOTER_FATAL);
    for (i = 0; lpeek(filename + i); ++i)
        POKE(FOOTER_ADDRESS + 44 + i, lpeek(filename + i));
    POKE(FOOTER_ADDRESS + 44 + i, ':');
    ++i;
    screen_decimal(FOOTER_ADDRESS + 44 + i, line_number);
    lfill(COLOUR_RAM_ADDRESS - SCREEN_ADDRESS + FOOTER_ADDRESS, 2 | ATTRIB_REVERSE, 80);
    for (;;)
        continue;
}

void set_screen_attributes(ptr_t p, uint8_t count, uint8_t attr)
{
    static uint32_t i;

    // This involves setting colour RAM values, so we need to either LPOKE, or
    // map the 2KB colour RAM in at $D800 and work with it there.
    // XXX - For now we are LPOKING
    addr = COLOUR_RAM_ADDRESS - SCREEN_ADDRESS + p;
    for (i = 0; i < count; ++i)
    {
        lpoke(addr, lpeek(addr) | attr);
        ++addr;
    }
}

// char read_line(char *buffer, uint8_t maxlen)
// {
//     char len = 0;
//     char c;
//     char reverse = 0x90;

//     // Read input using hardware keyboard scanner

//     while (len < maxlen)
//     {
//         c = *(charptr_t)0xD610;

// #if 0
//         reverse ^=0x20;
// #endif

//         // Show cursor
//         lpoke(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS,
//                     reverse |
//                             (lpeek(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS) & 0xf));

//         if (c)
//         {

//             if (c == 0x14)
//             {
//                 // DELETE
//                 if (len)
//                 {
//                     // Remove blink attribute from this char
//                     lpoke(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS,
//                                 lpeek(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS) & 0xf);

//                     // Go back one and erase
//                     len--;
//                     lpoke(screen_line_address + len, ' ');

//                     // Re-enable blink for cursor
//                     lpoke(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS,
//                                 lpeek(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS) | reverse);
//                     buffer[len] = 0;
//                 }
//             }
//             else if (c == 0x0d)
//             {
//                 buffer[len] = 0;
//                 return len;
//             }
//             else
//             {
//                 lpoke(screen_line_address + len, c);
//                 // Remove blink attribute from this char
//                 lpoke(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS,
//                             lpeek(len + screen_line_address + COLOUR_RAM_ADDRESS - SCREEN_ADDRESS) & 0xf);
//                 buffer[len++] = c;
//             }

//             //      *(charptr_t)0x8000 = c;

//             // Clear keys from hardware keyboard scanner
//             // XXX we clear all keys here, and work around a bug that causes crazy
//             // fast key repeating. This can be turned back into acknowledging the
//             // single key again later
//             while (*(charptr_t)0xD610)
//             {
//                 uint16_t i;
//                 *(charptr_t)0xd610 = 1;

//                 for (i = 0; i < 25000; ++i)
//                     continue;
//             }
//         }
//     }

//     return len;
// }
