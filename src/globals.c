#include "globals.h"

int8_t char_size = sizeof(uint8_t);
int32_t screen_width = 80;
int32_t screen_height = 24;
int32_t cursor_position = 0;
uint16_t cursor_attrib = 0U;
uint16_t char_attrib = 0U;
uint32_t screen_address = 0x8000U;
uint32_t colour_address = 0x1F800U;