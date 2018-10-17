#include "globals.h"

int32_t cursor_position = 0;
uint8_t cursor_attrib[2] = {0U, 0U};

int8_t char_size = sizeof(uint8_t);
uint8_t char_attrib[2] = {0U, 0U};

uint32_t screen_address = 0x8000U;
uint32_t colour_address = 0x1F800U;
uint32_t screen_width = 80 * sizeof(uint8_t); // 80 * char_size
uint32_t screen_size = 24 * 80 * sizeof(uint8_t); // 24 * screen_size

uint8_t c, x, y, z;
uint32_t i, j, k, l;

// uint32_t full_screen_buffer = 0x24000U;
// uint32_t full_screen_buffer_size = 0x26000U - 0x24000U;
// uint32_t full_colour_buffer = 0x26000U;
// uint32_t full_colour_buffer_size = 0x28000U - 0x26000U;

// uint32_t small_screen_buffer = 0x3C000U;
// uint32_t small_screen_buffer_size = 0x3D000U - 0x3C000U;
// uint32_t small_colour_buffer = 0x3D000U;
// uint32_t small_colour_buffer_size = 0x3E000U - 0x3D000U;