#include "f65.h"
#include "memory.h"

/*
  NOTE: Fonts are stored outside of the 64KB address space.
  Therefore you cannot use pointer types to point to them, because
  CC65 doesn't understand them, and will use 16-bit pointers.
  In fact, CC65 will do a HORRIBLE job at any access to memory >64KB,
  in part because it doesn't know about the 32-bit indirect addressing
  mode of the MEGA65's CPU.
*/
uint8_t x, y;
uint32_t i, j, k;
uint32_t prev_font = 0; // read only
uint16_t glyph_count = 0;
uint16_t tile_map_start = 0;
uint32_t point_list = 0;
uint32_t tile_array_start = 0U;
uint32_t tile_map_size = 0U, cards = 0U;
uint32_t point_list_length = 0U;
uint32_t map = 0, tile = 0;
uint32_t screen = 0, colour = 0;
uint8_t bytes_per_row;
uint8_t rows_above;
uint8_t rows_below;
uint32_t map_pos, array_pos;
uint32_t glyph_height, glyph_width, glyph_size;
uint8_t trim_pixels;
uint16_t the_code_point = 0, card;

uint8_t clear_pattern[4] = {0x20, 0x00, 0x20, 0x00};
uint8_t end_of_line_pattern[2] = {0xff, 0xff};

void renderTextUTF16(uint32_t font_address, uint16_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    for (x = 0; string[x]; ++x)
        renderGlyph(font_address, string[x], scratch,
                    colour_and_attributes, // Various colours
                    alpha_and_extras       // alpha blend
        );
}

void renderTextASCII(uint32_t font_address, uint8_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    for (x = 0; string[x]; ++x)
        renderGlyph(font_address, string[x], scratch,
                    colour_and_attributes, // Various colours
                    alpha_and_extras       // alpha blend
        );
}

void clearRenderBuffer(render_buffer_t *buffer)
{
    if (!buffer)
        return;

    // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
    lcopy((long)&clear_pattern, buffer->screen_ram, 4);
    // Fill out to whole line
    lcopy(buffer->screen_ram, buffer->screen_ram + 4, 196);
    // Then put end of line marker to stop displaying tiles from next line
    lcopy((long)&end_of_line_pattern, buffer->screen_ram + 198, 2);
    // Then copy it down over the next 59 rows.
    lcopy(buffer->screen_ram, buffer->screen_ram + 200, 11800);
}

void outputLineToRenderBuffer(render_buffer_t *in, render_buffer_t *out)
{
    if (!in)
        return;
    if (!out)
        return;
    if ((in->max_above + in->max_below) < 1)
        return;

    // Get total number of rows we need to actually output
    rows_below = in->max_above + in->max_below;
    if ((rows_below + out->rows_used) > 59)
        rows_below = 59 - out->rows_used;

    // And work out which is the first one
    rows_above = in->baseline_row - in->max_above;

    // Do the copies via DMA
    lcopy(in->screen_ram + rows_above * 200, out->screen_ram + 200 * out->rows_used, rows_below * 200);
    lcopy(in->colour_ram + rows_above * 200, out->colour_ram + 200 * out->rows_used, rows_below * 200);

    POKE(0x0400U, rows_above);
    POKE(0x0401U, rows_below);
    POKE(0x0402U, out->rows_used);

    // Mark the rows used in the output buffer
    out->rows_used += rows_below;
}

void findFontStructures(uint32_t font_address)
{
    prev_font = font_address;

    lcopy(font_address + 0x80, (long)&glyph_count, 2);

    lcopy(font_address + 0x82, (long)&tile_map_start, 2);
    point_list_length = tile_map_start - 0x100;
    point_list = font_address + 0x100;
    tile_array_start = 0;
    lcopy(font_address + 0x84, (long)&tile_array_start, 3);
    tile_map_size = font_address + tile_map_start - tile_array_start;
    map = font_address + tile_map_start;
}

void renderGlyph(uint32_t font_address, uint16_t code_point, render_buffer_t *b, uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    if (!b)
        return;
    if (prev_font != font_address)
        findFontStructures(font_address);

    // Default to allowing 24 rows above and 6 rows below for underhangs
    if (!b->baseline_row)
        b->baseline_row = 24;

    // XXX - Code points are in numerical order, so speed this up with
    // a binary search.
    for (i = 0; i < point_list_length; i += 5)
    {
        lcopy(point_list + i, (long)&the_code_point, 2);
        if (the_code_point != code_point)
            continue;

        // We have the glyph, so dig out the information on it.
        map_pos = 0;
        lcopy(point_list + i + 2, (long)&map_pos, 3);

        rows_above = lpeek(map_pos);
        rows_below = lpeek(map_pos + 1);
        bytes_per_row = lpeek(map_pos + 2);
        trim_pixels = lpeek(map_pos + 3);

        // If glyph is 0 pixels wide, nothing to do.
        if (!bytes_per_row)
            continue;
        // If glyph would overrun the buffer, don't draw it.
        if ((bytes_per_row + b->columns_used) > 99)
            continue;
        bytes_per_row = bytes_per_row << 1;

        // Don't allow glyphs that go too far above base line
        if (rows_above >= b->baseline_row)
            break;
        if (rows_below >= (30 - b->baseline_row))
            break;

        // Skip header
        map_pos += 4;

        // Work out first address of screen and colour RAM

        // First, work out the address of the row
        screen = b->screen_ram;
        colour = b->colour_ram;
        for (y = rows_above; y < b->baseline_row; ++y)
        {
            screen += 200;
            colour += 200;
        }
        // then advance to the first unused column
        screen += b->columns_used;
        screen += b->columns_used;
        colour += b->columns_used;
        colour += b->columns_used;

        // Now we need to copy each row of data to the screen RAM and colour RAM buffers
        for (y = 0; y < (rows_above + rows_below); ++y)
        {

            // Copy screen data
            lcopy((long)map_pos, (long)screen, bytes_per_row);

            // Fill colour RAM.
            // Bit 5 of low byte is alpha blending flag. This is true for font glyphs, and false
            // for graphics tiles.
            // Then bottom 4 bits of high byte is foreground colour, and the upper 4 bits are VIC-III
            // attributes (blink, underline etc)
            // This requires 0x20 for the first byte, to enable alpha blending
            lpoke(colour, alpha_and_extras);
            if (y == rows_above-1)
                lpoke(colour + 1, colour_and_attributes);
            else
                lpoke(colour + 1, colour_and_attributes & (~ATTRIB_UNDERLINE));
            if (bytes_per_row > 2)
            {
                lpoke(colour + 2, alpha_and_extras);
                if (y == rows_above-1)
                    lpoke(colour + 3, colour_and_attributes);
                else
                    lpoke(colour + 3, colour_and_attributes & (~ATTRIB_UNDERLINE));
            }
            // Then use DMA to copy the pair of bytes out
            // (DMA timing is a bit funny, hence we need to have the two copies setup above
            //  for this to work reliably).
            if (bytes_per_row > 4)
                lcopy((long)colour, (long)colour + 2, bytes_per_row - 2);

            map_pos += bytes_per_row;

            screen += 200;
            colour += 200;
        }

        b->columns_used += (bytes_per_row >> 1);
        if (rows_above > b->max_above)
            b->max_above = rows_above;
        if (rows_below > b->max_above)
            b->max_above = rows_below;

        // then apply trim to entire column
        trim_pixels = trim_pixels << 5;
        screen = b->screen_ram + b->columns_used + b->columns_used - 1;
        for (y = 0; y < 30; ++y)
        {
            lpoke(screen, lpeek(screen) | trim_pixels);
            screen += 200;
        }

        break;
    }
    if (i == point_list_length)
    {
        // Unknown Glyph, so draw a box with the code point number in it?
    }
}

void patchFont(uint32_t font_address)
{
    if (prev_font != font_address)
        findFontStructures(font_address);

    // Patch tile_array_start
    tile_array_start += font_address;
    lcopy((long)&tile_array_start, font_address + 0x84, 3);

    tile_array_start /= 0x40;

    for (i = 0; i < point_list_length; i += 5)
    {
        map_pos = 0;
        lcopy(point_list + i + 2, (long)&map_pos, 3);
        tile = map + map_pos;
        lcopy((long)&tile, point_list + i + 2, 3);
        cards = tile + 4;

        rows_above = lpeek(tile);
        rows_below = lpeek(tile + 1);
        bytes_per_row = lpeek(tile + 2);
        glyph_size = (int)(rows_above + rows_below) * bytes_per_row;
        for (j = 0; j < glyph_size; ++j)
        {
            lcopy(cards, (long)&card, 2);
            card += tile_array_start;
            lcopy((long)&card, cards, 2);
            cards += 2;
        }
    }
}
