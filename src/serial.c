#include "serial.h"

void console_write_pchar(uint8_t c)
{
    __asm__("lda (sp)");
    __asm__("sta $d643");
    __asm__("nop");
}

void console_write_pstr(uint8_t *str)
{
    for (m = 0; str[m]; ++m)
        console_write_pchar(str[m]);
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
    else if (c >= 0x30)
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
    for (m = 0; str[m]; ++m)
        console_write_achar(str[m]);
}