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

// TODO: replace 59 with screen_height-1

uint8_t font_id = 0;

uint8_t start_column;
uint32_t prev_font = 0U; // read only
uint16_t glyph_count = 0U;

// point list
uint32_t point_list_address = 0U, point_list_size = 0U;

// tile map
uint16_t tile_map_start = 0U;
uint32_t tile_map_address = 0U, tile_map_size = 0U;

// tile array
uint32_t tile_array_start = 0U, tile_array_address = 0U;

uint32_t screen = 0U, colour = 0U;
uint8_t bytes_per_row;
uint8_t rows_above;
uint8_t rows_below;
uint32_t map_pos, array_pos;
uint32_t glyph_height, glyph_width, glyph_size;
uint8_t trim_pixels;
uint16_t the_code_point = 0U;

uint8_t clear_pattern[4] = {0x20, 0x00, 0x20, 0x00};
uint8_t end_of_line_pattern[2] = {0xff, 0xff};

render_buffer_t screen_rbuffer;
render_buffer_t scratch_rbuffer;
render_buffer_t *active_rbuffer = 0;

void renderTextUTF16(uint32_t font_address, uint16_t *string, uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    for (x = 0; string[x]; ++x)
        renderGlyph(font_address, string[x],
                    colour_and_attributes, // Various colours
                    alpha_and_extras,      // alpha blend
                    0xFF                   // Append to end
        );
}

void renderTextASCII(uint32_t font_address, uint8_t *string, uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    for (x = 0; string[x]; ++x)
        renderGlyph(font_address, string[x],
                    colour_and_attributes, // Various colours
                    alpha_and_extras,      // alpha blend
                    0xFF                   // Append to end
        );
}

void clearRenderBuffer(void)
{
    if (!active_rbuffer)
        return;

    // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
    lcopy((long)&clear_pattern, active_rbuffer->screen_ram, sizeof(clear_pattern));
    // Fill out to whole line
    lcopy(active_rbuffer->screen_ram, active_rbuffer->screen_ram + sizeof(clear_pattern), (screen_width - sizeof(clear_pattern)));
    // Then put end of line marker to stop displaying tiles from next line
    lcopy((long)&end_of_line_pattern, active_rbuffer->screen_ram + (screen_width - sizeof(end_of_line_pattern)), sizeof(end_of_line_pattern));
    // Then copy it down over the next 59 rows.
    lcopy(active_rbuffer->screen_ram, active_rbuffer->screen_ram + screen_width, screen_size - screen_width);

    active_rbuffer->columns_used = 0;
    active_rbuffer->max_above = 0;
    active_rbuffer->max_below = 0;
    active_rbuffer->baseline_row = 24;
    active_rbuffer->trimmed_pixels = 0;
    active_rbuffer->glyph_count = 0;
}

void outputLineToRenderBuffer(void)
{
    if ((scratch_rbuffer.max_above + scratch_rbuffer.max_below) < 1)
        return;

    // Get total number of rows we need to actually output
    rows_below = scratch_rbuffer.max_above + scratch_rbuffer.max_below;
    if ((rows_below + screen_rbuffer.rows_used) > 59)
        rows_below = 59 - screen_rbuffer.rows_used;

    // And work out which is the first one
    rows_above = scratch_rbuffer.baseline_row - scratch_rbuffer.max_above;

    // Do the copies via DMA
    lcopy(scratch_rbuffer.screen_ram + rows_above * screen_width, screen_rbuffer.screen_ram + screen_width * screen_rbuffer.rows_used, rows_below * screen_width);
    lcopy(scratch_rbuffer.colour_ram + rows_above * screen_width, screen_rbuffer.colour_ram + screen_width * screen_rbuffer.rows_used, rows_below * screen_width);

    // Mark the rows used in the output buffer
    screen_rbuffer.rows_used += rows_below;
}

void findFontStructures(uint32_t font_address)
{
    prev_font = font_address;

    lcopy(font_address + 0x80, (long)&glyph_count, 2);

    lcopy(font_address + 0x82, (long)&tile_map_start, 2);
    point_list_size = tile_map_start - 0x100;
    point_list_address = font_address + 0x100;
    tile_array_start = 0;
    lcopy(font_address + 0x84, (long)&tile_array_start, 3);
    tile_map_size = font_address + tile_map_start - tile_array_start;
    tile_map_address = font_address + tile_map_start;
    tile_array_address = font_address + tile_array_start;
}

glyph_details_t *gd = 0;

void deleteGlyph(uint8_t glyph_num)
{
    // Make sure buffer and glyph number are ok
    if (!active_rbuffer)
        return;
    if (glyph_num >= active_rbuffer->glyph_count)
        return;

    rows_above = active_rbuffer->max_above;
    rows_below = active_rbuffer->max_below;

    // Get start address for the first row we have to copy.
    screen = active_rbuffer->screen_ram;
    colour = active_rbuffer->colour_ram;
    for (y = rows_above; y < active_rbuffer->baseline_row; ++y)
    {
        screen += screen_width;
        colour += screen_width;
    }
    // then advance to the first unused column
    screen += active_rbuffer->glyphs[glyph_num].first_column * 2;
    colour += active_rbuffer->glyphs[glyph_num].first_column * 2;
    x = active_rbuffer->glyphs[glyph_num].columns * 2;
    bytes_per_row = (active_rbuffer->columns_used - active_rbuffer->glyphs[glyph_num].first_column) * 2;

    // Copy remaining glyphs on the line
    if ((glyph_num + 1) != active_rbuffer->glyph_count)
    {
        for (y = 0; y < (rows_above + rows_below); ++y)
        {
            // Copy screen data
            lcopy(screen + x, screen, bytes_per_row);
            // Copy colour data
            lcopy(colour + x, colour, bytes_per_row);

            screen += screen_width;
            colour += screen_width;
        }
    }

    // Note if we need to recalculate rows above and rows below
    x = (active_rbuffer->glyphs[glyph_num].rows_above == rows_above) ||
        (active_rbuffer->glyphs[glyph_num].rows_below == rows_below)
        ? 1 : 0;

    // Subtract the used columns
    active_rbuffer->columns_used -= active_rbuffer->glyphs[glyph_num].columns;

    // Now erase the tail of each line
    screen = active_rbuffer->screen_ram + active_rbuffer->columns_used * 2;
    colour = active_rbuffer->colour_ram + active_rbuffer->columns_used * 2;
    bytes_per_row = active_rbuffer->glyphs[glyph_num].columns * 2;

    for (y = rows_above; y < active_rbuffer->baseline_row; ++y)
    {
        screen += screen_width;
        colour += screen_width;
    }

    for (y = 0; y < (rows_above + rows_below); ++y)
    {
        // Clear screen data
        lfill(screen, 0x20, bytes_per_row);
        // Clear colour data
        lfill(colour, 0x00, bytes_per_row);

        screen += screen_width;
        colour += screen_width;
    }

    // Now copy down the glyph details structure.
    // We also do a DMA here for speed
    lcopy((uint32_t)&active_rbuffer->glyphs[glyph_num + 1], (uint32_t)&active_rbuffer->glyphs[glyph_num], 99 - glyph_num);

    // Reduce number of remaining glyphs
    active_rbuffer->glyph_count--;

    // Update rows_above and rows_below if require
    if (x)
    {
        rows_above = 0;
        rows_below = 0;
        for (y = 0; y < active_rbuffer->glyph_count; y++)
        {
            if (active_rbuffer->glyphs[y].rows_above > rows_above)
                rows_above = active_rbuffer->glyphs[y].rows_above;
            if (active_rbuffer->glyphs[y].rows_below > rows_below)
                rows_below = active_rbuffer->glyphs[y].rows_below;
        }
        active_rbuffer->max_above = rows_above;
        active_rbuffer->max_below = rows_below;
    }
}

void replaceGlyph(uint8_t glyph_num, uint32_t font_address, uint16_t code_point);
void insertGlyph(uint8_t glyph_num, uint32_t font_address, uint16_t code_point);

void renderGlyph(uint32_t font_address, uint16_t code_point, uint8_t colour_and_attributes, uint8_t alpha_and_extras, uint8_t position)
{
    if (!active_rbuffer)
        return;
    if (prev_font != font_address)
        findFontStructures(font_address);

    // Default to allowing 24 rows above and 6 rows below for underhangs
    if (!active_rbuffer->baseline_row)
        active_rbuffer->baseline_row = 24;

    if (position == 0xFF)
        position = active_rbuffer->glyph_count;

    // Make space if this glyph is not at the end
    if (position < active_rbuffer->glyph_count)
        start_column = active_rbuffer->glyphs[position].first_column;
    else
    {
        start_column = active_rbuffer->columns_used;
    }

    if (start_column > 99)
    {
        return;
    }

    // XXX - Code points are in numerical order, so speed this up with
    // a binary search.
    for (i = 0; i < point_list_size; i += 5)
    {
        lcopy(point_list_address + i, (long)&the_code_point, 2);
        if (the_code_point != code_point)
            continue;

        // We have the glyph, so dig out the information on it.
        map_pos = 0;
        lcopy(point_list_address + i + 2, (long)&map_pos, 3);

        rows_above = lpeek(map_pos);
        rows_below = lpeek(map_pos + 1);
        bytes_per_row = lpeek(map_pos + 2);
        trim_pixels = lpeek(map_pos + 3);

        // If glyph is 0 pixels wide, nothing to do.
        if (!bytes_per_row)
            continue;
        // If glyph would overrun the buffer, don't draw it.
        if ((bytes_per_row + active_rbuffer->columns_used) > 99)
            continue;

        // Don't allow glyphs that go too far above base line
        if (rows_above >= active_rbuffer->baseline_row)
            break;
        if (rows_below >= (30 - active_rbuffer->baseline_row))
            break;

        // Ok, we have a glyph, so now we make space in the glyph list
        // This is an overlapping copy, so we have to copy to a temp location first
        if (position < active_rbuffer->glyph_count)
            lcopy_safe((long)&active_rbuffer->glyphs[position], (long)&active_rbuffer->glyphs[position + 1],
                sizeof(glyph_details_t) * (active_rbuffer->glyph_count - position));

        // Record details about this glyph
        gd = &active_rbuffer->glyphs[position];
        gd->code_point = code_point;
        gd->font_id = 0; // XXX - Fix when we support multiple loaded fonts
        gd->rows_above = rows_above;
        gd->rows_below = rows_below;
        gd->columns = bytes_per_row;
        gd->trim_pixels = trim_pixels;
        gd->first_column = start_column;
        gd->colour_and_attributes = colour_and_attributes;

        bytes_per_row = bytes_per_row << 1;

        // Skip header
        map_pos += 4;

        if (position < active_rbuffer->glyph_count)
        {
            // Char is not on the end, so make space

            // First, work out the address of the start row and column
            screen = active_rbuffer->screen_ram + start_column * 2;
            colour = active_rbuffer->colour_ram + start_column * 2;
            for (y = active_rbuffer->max_above; y < active_rbuffer->baseline_row; ++y)
            {
                screen += screen_width;
                colour += screen_width;
            }

            // Now we need to copy each row of data to the screen RAM and colour RAM buffers
            for (y = 0; y < (active_rbuffer->max_above + active_rbuffer->max_below); ++y)
            {
                // Make space
                lcopy_safe(screen, screen + bytes_per_row, (active_rbuffer->columns_used - start_column) * 2);
                lcopy_safe(colour, colour + bytes_per_row, (active_rbuffer->columns_used - start_column) * 2);

                // Fill screen and colour RAM with empty patterns
                lcopy((long)&clear_pattern, screen, bytes_per_row);
                lcopy(screen, screen + 2, bytes_per_row - 2);
                lfill(colour, 0x00, bytes_per_row);

                screen += screen_width;
                colour += screen_width;
            }
        }

        // Work out first address of screen and colour RAM

        // First, work out the address of the start row and column
        screen = active_rbuffer->screen_ram + start_column * 2;
        colour = active_rbuffer->colour_ram + start_column * 2;
        for (y = rows_above; y < active_rbuffer->baseline_row; ++y)
        {
            screen += screen_width;
            colour += screen_width;
        }

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
            if (y == rows_above - 1)
                lpoke(colour + 1, colour_and_attributes);
            else
                lpoke(colour + 1, colour_and_attributes & (~ATTRIB_UNDERLINE));
            if (bytes_per_row > 2)
            {
                lpoke(colour + 2, alpha_and_extras);
                if (y == rows_above - 1)
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

            screen += screen_width;
            colour += screen_width;
        }

        active_rbuffer->glyph_count++;
        active_rbuffer->columns_used += (bytes_per_row >> 1);
        if (rows_above > active_rbuffer->max_above)
            active_rbuffer->max_above = rows_above;
        if (rows_below > active_rbuffer->max_below)
            active_rbuffer->max_below = rows_below;

        // then apply trim to entire column
        trim_pixels = trim_pixels << 5;
        screen = active_rbuffer->screen_ram + start_column + start_column + bytes_per_row - 1;
        for (y = 0; y < 30; ++y)
        {
            lpoke(screen, (lpeek(screen) & 0x1f) | trim_pixels);
            screen += screen_width;
        }

        break;
    }
    if (i == point_list_size)
    {
        // Unknown Glyph, so draw a box with the code point number in it?
    }
}

uint32_t point_tile, tile_address, tile_cards;
uint16_t card_address;
void patchFont(uint32_t font_address)
{
    // if (prev_font == font_address) return;

    findFontStructures(font_address);

    // Patch tile_array_start
    tile_array_start += font_address;
    lcopy((long)&tile_array_start, font_address + 0x84, 3);

    tile_array_start /= 0x40;

    for (i = 0; i < point_list_size; i += 5)
    {
        // map_pos = 0;
        // lcopy(point_list + i + 2, (long)&map_pos, 3);
        // tile = map + map_pos;
        tile_address = 0;
        point_tile = point_list_address + i + 2;
        lcopy(point_tile, (long)&tile_address, 3);
        tile_address += tile_map_address;
        lcopy((long)&tile_address, point_tile, 3);

        rows_above = lpeek(tile_address);
        rows_below = lpeek(tile_address + 1);
        bytes_per_row = lpeek(tile_address + 2);
        // tile_address + 3 -> trim_pixels
        tile_cards = tile_address + 4;
        glyph_size = (int)(rows_above + rows_below) * bytes_per_row;
        for (j = 0; j < glyph_size; ++j)
        {
            lcopy(tile_cards, (long)&card_address, 2);
            // lcopy(gradient, (((uint32_t)card_address&0x0FFF) * 64) + tile_array_address, 64);
            card_address += tile_array_start;
            lcopy((long)&card_address, tile_cards, 2);
            tile_cards += 2;
        }
    }
}
