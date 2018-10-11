#include <stdint.h>

#include "globals.h"

typedef struct render_buffer
{
    uint32_t screen_ram;
    uint32_t colour_ram;
    uint8_t columns_used; // only used when building a line of output
    uint8_t rows_used;    // only for when pasting lines from another buffer
    uint8_t max_above;
    uint8_t max_below;
    uint8_t baseline_row;
    uint16_t trimmed_pixels; // total number of trimmed pixels
} render_buffer_t;

void renderGlyph(uint32_t font_address, uint16_t code_point, render_buffer_t *b,
                uint8_t colour_and_attributes, uint8_t alpha_and_extras);
void findFontStructures(uint32_t font_address);
void patchFont(uint32_t font_address);
void clearRenderBuffer(render_buffer_t *buffer);
void outputLineToRenderBuffer(render_buffer_t *in, render_buffer_t *out);
void renderTextUTF16(uint32_t font_address, uint16_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras);
void renderTextASCII(uint32_t font_address, uint8_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras);
