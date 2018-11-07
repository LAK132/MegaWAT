#include "megaint.h"
#include "memory.h"

extern uint8_t console_indent;

void console_write_pchar(uint8_t c);
void console_write_pstr(uint8_t *str);

void console_write_achar(uint8_t c);
void console_write_astr(uint8_t *str);
void console_write_au8(uint8_t n);
void console_write_au16(uint16_t n);
void console_write_au32(uint32_t n);

#define console_write_newline() console_write_astr("\r\n")