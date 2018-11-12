#include "globals.h"

int32_t cursor_position = 0;
uint8_t cursor_attrib[2] = {0U, 0U};

int8_t char_size = sizeof(uint8_t);
uint8_t char_attrib[2] = {0U, 0U};

uint32_t screen_width = 80 * sizeof(uint8_t); // 80 * char_size
uint32_t screen_size = 24 * 80 * sizeof(uint8_t); // 24 * screen_size

int8_t s, r, t;
uint8_t c, x, y, z, w;
uint32_t i, j, k, l, m;
