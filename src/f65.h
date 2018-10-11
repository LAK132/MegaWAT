#include <stdint.h>

#include "globals.h"

struct render_buffer {
  uint32_t screen_ram;
  uint32_t colour_ram;
  uint8_t columns_used;
  uint8_t max_above;
  uint8_t max_below;
  uint8_t baseline_row;
  uint16_t trimmed_pixels;  // total number of trimmed pixels
};

void renderGlyph(uint32_t font_address,uint16_t code_point, struct render_buffer *b,
		 uint8_t colour_and_attributes,uint8_t alpha_and_extras);
void findFontStructures(uint32_t font_address);
void patchFont(uint32_t font_address);
void clearRenderBuffer(struct render_buffer *buffer);
