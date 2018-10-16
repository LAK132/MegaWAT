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
uint8_t x, y, start_column;
uint32_t i, j, k;
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
// uint8_t gradient[] = {0x0, 0x1F, 0x3F, 0x5F, 0x9F, 0xBF, 0xDF, 0xFF};
uint8_t gradient[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
};

void renderTextUTF16(uint32_t font_address, uint16_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
  for (x = 0; string[x]; ++x) {
    renderGlyph(font_address, string[x], scratch,
		colour_and_attributes, // Various colours
		alpha_and_extras,       // alpha blend
		0xFF   // Append to end
		);
  }
}

void renderTextASCII(uint32_t font_address, uint8_t *string, render_buffer_t *scratch,
                    uint8_t colour_and_attributes, uint8_t alpha_and_extras)
{
    for (x = 0; string[x]; ++x)
        renderGlyph(font_address, string[x], scratch,
                    colour_and_attributes, // Various colours
                    alpha_and_extras,       // alpha blend
		    0xFF // Append to end
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

    buffer->columns_used = 0;
    buffer->max_above = 0;
    buffer->max_below = 0;
    buffer->baseline_row = 24;
    buffer->trimmed_pixels = 0;
    buffer->glyph_count=0;
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

    // Mark the rows used in the output buffer
    out->rows_used += rows_below;
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

glyph_details_t *gd=0;

void deleteGlyph(render_buffer_t *b,uint8_t glyph_num)
{
  // Make sure buffer and glyph number are ok
  if (!b) return;
  if (glyph_num>=b->glyph_count) return;

  rows_above=b->max_above;
  rows_below=b->max_below;

  // Get start address for the first row we have to copy.
  screen = b->screen_ram;
  colour = b->colour_ram;
  for (y = rows_above; y < b->baseline_row; ++y)
    {
      screen += 200;
      colour += 200;
    }
  // then advance to the first unused column
  screen += b->glyphs[glyph_num].first_column*2;
  colour += b->glyphs[glyph_num].first_column*2;
  x = b->glyphs[glyph_num].columns*2;
  bytes_per_row = (b->columns_used - b->glyphs[glyph_num].first_column)*2;

  // Copy remaining glyphs on the line
  if ((glyph_num+1)!=b->glyph_count) {
    for (y = 0; y < (rows_above + rows_below); ++y)
      {
	
	// Copy screen data
	lcopy(screen+x, screen, bytes_per_row);
	// Copy colour data
	lcopy(colour+x, colour, bytes_per_row);
	
	screen+=200;
	colour+=200;
	
      }
  }

  // Note if we need to recalculate rows above and rows below
  if ((b->glyphs[glyph_num].rows_above==rows_above)
      ||(b->glyphs[glyph_num].rows_below==rows_below))
    x=1; else x=0;
  
  // Subtract the used columns
  b->columns_used-=b->glyphs[glyph_num].columns;

  // Now erase the tail of each line
  screen = b->screen_ram + b->columns_used*2;
  colour = b->colour_ram + b->columns_used*2;
  bytes_per_row= b->glyphs[glyph_num].columns*2;  

  for (y = rows_above; y < b->baseline_row; ++y)
    {
      screen += 200;
      colour += 200;
    }
  
  for (y = 0; y < (rows_above + rows_below); ++y)
    {
      // Clear screen data
      lfill(screen, 0x20, bytes_per_row);
      // Clear colour data
      lfill(colour,0x00, bytes_per_row);
      
      screen+=200;
      colour+=200;
    }

  // Now copy down the glyph details structure.
  // We also do a DMA here for speed
  lcopy((uint32_t)&b->glyphs[glyph_num+1],(uint32_t)&b->glyphs[glyph_num],99-glyph_num);

  // Reduce number of remaining glyphs
  b->glyph_count--;

  // Update rows_above and rows_below if require
  if (x) {
    rows_above=0;
    rows_below=0;
    for(y=0;y<b->glyph_count;y++)
      {
	if (b->glyphs[y].rows_above>rows_above) rows_above=b->glyphs[y].rows_above;
	if (b->glyphs[y].rows_below>rows_below) rows_below=b->glyphs[y].rows_below;
      }
    b->max_above=rows_above;
    b->max_below=rows_below;
  }

}

void replaceGlyph(render_buffer_t *b,uint8_t glyph_num, uint32_t font_address, uint16_t code_point);
void insertGlyph(render_buffer_t *b,uint8_t glyph_num, uint32_t font_address, uint16_t code_point);

void renderGlyph(uint32_t font_address, uint16_t code_point, render_buffer_t *b, uint8_t colour_and_attributes,
		 uint8_t alpha_and_extras, uint8_t position)
{
    if (!b)
        return;
    if (prev_font != font_address)
        findFontStructures(font_address);

    // Default to allowing 24 rows above and 6 rows below for underhangs
    if (!b->baseline_row)
        b->baseline_row = 24;

    if (position==0xFF) position=b->glyph_count;
    
    // Make space if this glyph is not at the end
    if (position<b->glyph_count)
      start_column=b->glyphs[position].first_column;
    else {
      start_column=b->columns_used;
    }

    if (start_column>99) {
      POKE(0xD020U,0);
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
        if ((bytes_per_row + b->columns_used) > 99)
            continue;

        // Don't allow glyphs that go too far above base line
        if (rows_above >= b->baseline_row)
            break;
        if (rows_below >= (30 - b->baseline_row))
            break;

	// Ok, we have a glyph, so now we make space in the glyph list
	// This is an overlapping copy, so we have to copy to a temp location first
	if (position<b->glyph_count)
	  lcopy_safe(&b->glyphs[position],&b->glyphs[position+1],sizeof(glyph_details_t)*(b->glyph_count-position));
	
	// Record details about this glyph	
	gd=&b->glyphs[position];
	gd->code_point=code_point;
	gd->font_id=0; // XXX - Fix when we support multiple loaded fonts
	gd->rows_above=rows_above;
	gd->rows_below=rows_below;
	gd->columns=bytes_per_row;
	gd->trim_pixels=trim_pixels;
	gd->first_column=start_column;

        bytes_per_row = bytes_per_row << 1;
	
        // Skip header
        map_pos += 4;

	if (position<b->glyph_count) {
	    // Char is not on the end, so make space

	  // First, work out the address of the start row and column
	  screen = b->screen_ram + start_column*2;
	  colour = b->colour_ram + start_column*2;
	  for (y = b->max_above; y < b->baseline_row; ++y)
	    {
	      screen += 200;
	      colour += 200;
	    }

	  // Now we need to copy each row of data to the screen RAM and colour RAM buffers
	  for (y = 0; y < (b->max_above + b->max_below); ++y)
	    {
	      // Make space
	      lcopy_safe(screen,screen+bytes_per_row,(b->columns_used - start_column)*2);
	      lcopy_safe(colour,colour+bytes_per_row,(b->columns_used - start_column)*2);

	      // Fill screen and colour RAM with empty patterns
	      lcopy((long)&clear_pattern,screen,bytes_per_row);
	      lcopy(screen,screen+2,bytes_per_row-2);
	      lfill(colour,0x00,bytes_per_row);
	      
	      screen += 200;
	      colour += 200;
	    }
	}


	
        // Work out first address of screen and colour RAM

        // First, work out the address of the start row and column
        screen = b->screen_ram + start_column*2;
        colour = b->colour_ram + start_column*2;
        for (y = rows_above; y < b->baseline_row; ++y)
        {
            screen += 200;
            colour += 200;
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

            screen += 200;
            colour += 200;
        }

	b->glyph_count++;
        b->columns_used += (bytes_per_row >> 1);
        if (rows_above > b->max_above)
            b->max_above = rows_above;
        if (rows_below > b->max_below)
            b->max_below = rows_below;

        // then apply trim to entire column
        trim_pixels = trim_pixels << 5;
        screen = b->screen_ram + start_column + start_column + bytes_per_row - 1;
        for (y = 0; y < 30; ++y)
        {
            lpoke(screen, (lpeek(screen) & 0x1f) | trim_pixels);
            screen += 200;
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
