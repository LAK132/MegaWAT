#include <stdint.h>
#include "globals.h"

#ifndef F65_H
#define F65_H

extern uint8_t font_id;

// Try to keep this structure 8 bytes = a power of two for efficiency
typedef struct glyph_details {
    uint16_t code_point;
    uint8_t font_id;
    uint8_t rows_above;
    uint8_t rows_below;
    uint8_t columns;
    uint8_t trim_pixels;
    uint8_t first_column;
    uint8_t colour_and_attributes;
} glyph_details_t;

typedef struct render_buffer
{
    // Address of screen RAM 100x60x(2 bytes) buffer
    uint32_t screen_ram;
    // Address of colour RAM 100x60x(2 bytes) buffer
    uint32_t colour_ram;
    uint8_t columns_used; // only used when building a line of output
    uint8_t rows_used;    // only for when pasting lines from another buffer
    uint8_t max_above;
    uint8_t max_below;
    uint8_t baseline_row;
    uint16_t trimmed_pixels; // total number of trimmed pixels

    // List of glyphs already displayed, their positions and sizes, so that
    // we can easily insert and delete characters in the rendered buffer
    glyph_details_t glyphs[100];
    uint8_t glyph_count;
} render_buffer_t;

extern render_buffer_t screen_rbuffer;
extern render_buffer_t scratch_rbuffer;
extern render_buffer_t *active_rbuffer;

void deleteGlyph(uint8_t glyph_num);
void renderGlyph(uint32_t font_address, uint16_t code_point, uint8_t colour_and_attributes, uint8_t alpha_and_extras, uint8_t position);
void findFontStructures(uint32_t font_address);
void patchFont(uint32_t font_address);
void clearRenderBuffer(void);
void outputLineToRenderBuffer(void);
void renderTextUTF16(uint32_t font_address, uint16_t *string, uint8_t colour_and_attributes, uint8_t alpha_and_extras);
void renderTextASCII(uint32_t font_address, uint8_t *string, uint8_t colour_and_attributes, uint8_t alpha_and_extras);

#endif // F65_H