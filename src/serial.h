#include "globals.h"
#include "memory.h"

void console_write_pchar(uint8_t c);
void console_write_pstr(uint8_t *str);
void console_write_achar(uint8_t c);
void console_write_astr(uint8_t *str);
#define console_write_newline() console_write_astr("\r\n")