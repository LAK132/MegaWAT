#include "serial.h"

uint8_t console_indent = 0;

void console_write_pchar(uint8_t c)
{
    __asm__("lda (sp)");
    __asm__("sta $d643");
    __asm__("nop");
}

int32_t ii;
void console_write_pstr(uint8_t *str)
{
    for (ii = 0; ii < console_indent; ++ii)
        console_write_pchar(' ');
    for (ii = 0; str[ii]; ++ii)
        console_write_pchar(str[ii]);
}

void console_write_achar(uint8_t c)
{
    if (c >= 0x60)
    {
        __asm__("lda (sp)");
        __asm__("sbc #$80");
        __asm__("sta $d643");
        __asm__("nop");
    }
    else if (c > 0x40)
    {
        __asm__("lda (sp)");
        __asm__("adc #$1F");
        __asm__("sta $d643");
        __asm__("nop");
    }
    else
    {
        __asm__("lda (sp)");
        __asm__("sta $d643");
        __asm__("nop");
    }
}

void console_write_astr(uint8_t *str)
{
    for (ii = 0; ii < console_indent; ++ii)
        console_write_achar(' ');
    for (ii = 0; str[ii]; ++ii)
        console_write_achar(str[ii]);
}

uint8_t cc[8];
void console_write_au8(uint8_t n)
{
    console_write_achar(' ');
    console_write_achar('0');
    console_write_achar('x');
    for (ii = 0; ii < 2; ++ii)
        cc[ii] = (n >> (ii * 4)) & 0xF;
    for (--ii; ii >= 0; --ii)
        if (cc[ii] >= 0xA)
            console_write_achar('A' + (cc[ii] - 0xA));
        else
            console_write_achar('0' + cc[ii]);
}

void console_write_au16(uint16_t n)
{
    console_write_achar(' ');
    console_write_achar('0');
    console_write_achar('x');
    for (ii = 0; ii < 4; ++ii)
        cc[ii] = (n >> (ii * 4)) & 0xF;
    for (--ii; ii >= 0; --ii)
        if (cc[ii] >= 0xA)
            console_write_achar('A' + (cc[ii] - 0xA));
        else
            console_write_achar('0' + cc[ii]);
}

void console_write_au32(uint32_t n)
{
    console_write_achar(' ');
    console_write_achar('0');
    console_write_achar('x');
    for (ii = 0; ii < 8; ++ii)
        cc[ii] = (n >> (ii * 4)) & 0xF;
    for (--ii; ii >= 0; --ii)
        if (cc[ii] >= 0xA)
            console_write_achar('A' + (cc[ii] - 0xA));
        else
            console_write_achar('0' + cc[ii]);
}