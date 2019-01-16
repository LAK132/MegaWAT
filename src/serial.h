// #include "megaint.h"
// #include "memory.h"

// #ifndef SERIAL_H
// #define SERIAL_H

// static uint8_t console_indent;

// static void console_write_pchar(uint8_t c)
// {
//     __asm__("lda (sp)");
//     __asm__("sta $d643");
//     __asm__("nop");
// }

// static void console_write_pstr(uint8_t *str)
// {
//     static int32_t i;
//     for (i = 0; i < console_indent; ++i)
//         console_write_pchar(' ');
//     for (i = 0; str[i]; ++i)
//         console_write_pchar(str[i]);
// }

// static void console_write_achar(uint8_t c)
// {
//     if (c >= 0x60)
//     {
//         __asm__("lda (sp)");
//         __asm__("sbc #$80");
//         __asm__("sta $d643");
//         __asm__("nop");
//     }
//     else if (c > 0x40)
//     {
//         __asm__("lda (sp)");
//         __asm__("adc #$1F");
//         __asm__("sta $d643");
//         __asm__("nop");
//     }
//     else
//     {
//         __asm__("lda (sp)");
//         __asm__("sta $d643");
//         __asm__("nop");
//     }
// }

// static void console_write_astr(uint8_t *str)
// {
//     static int32_t i;
//     for (i = 0; i < console_indent; ++i)
//         console_write_achar(' ');
//     for (i = 0; str[i]; ++i)
//         console_write_achar(str[i]);
// }

// static uint8_t cc[8];
// static void console_write_au8(uint8_t n)
// {
//     static int32_t i;
//     console_write_achar(' ');
//     console_write_achar('0');
//     console_write_achar('x');
//     for (i = 0; i < 2; ++i)
//         cc[i] = (n >> (i * 4)) & 0xF;
//     for (--i; i >= 0; --i)
//         if (cc[i] >= 0xA)
//             console_write_achar('A' + (cc[i] - 0xA));
//         else
//             console_write_achar('0' + cc[i]);
// }

// static void console_write_au16(uint16_t n)
// {
//     static int32_t i;

//     console_write_achar(' ');
//     console_write_achar('0');
//     console_write_achar('x');
//     for (i = 0; i < 4; ++i)
//         cc[i] = (n >> (i * 4)) & 0xF;
//     for (--i; i >= 0; --i)
//         if (cc[i] >= 0xA)
//             console_write_achar('A' + (cc[i] - 0xA));
//         else
//             console_write_achar('0' + cc[i]);
// }

// static void console_write_au32(uint32_t n)
// {
//     static int32_t i;

//     console_write_achar(' ');
//     console_write_achar('0');
//     console_write_achar('x');
//     for (i = 0; i < 8; ++i)
//         cc[i] = (n >> (i * 4)) & 0xF;
//     for (--i; i >= 0; --i)
//         if (cc[i] >= 0xA)
//             console_write_achar('A' + (cc[i] - 0xA));
//         else
//             console_write_achar('0' + cc[i]);
// }


// #define console_write_newline() console_write_astr("\r\n")

// #endif // SERIAL_H