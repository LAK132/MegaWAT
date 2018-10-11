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
uint8_t x,y;
uint32_t i,j,k;
uint16_t glyph_count=0;
uint16_t tile_map_start=0;
uint32_t point_list=0;
uint32_t tile_array_start=0U;
uint32_t tile_map_size=0U,cards=0U;
uint32_t point_list_length=0U;
uint32_t map=0,tile=0;
uint32_t screen=0,colour=0;
uint8_t bytes_per_row;
uint8_t rows_above;
uint8_t rows_below;
uint32_t map_pos, array_pos;
uint32_t glyph_height, glyph_width, glyph_size;
uint8_t trim_pixels;
uint16_t the_code_point=0,card;

void findFontStructures(uint32_t font_address)
{
  lcopy(font_address+0x80,(long)&glyph_count,2);

  tile_map_start=0; lcopy(font_address+0x82,(long)&tile_map_start,3);
  point_list_length = tile_map_start - 0x100;
  point_list = font_address + 0x100;
  tile_array_start=0; lcopy(font_address+0x84,(long)&tile_array_start,3);
  tile_map_size = tile_map_start - tile_array_start;
  map = font_address + tile_map_start;

}

void renderGlyph(uint32_t font_address,uint16_t code_point, struct render_buffer *b,uint8_t colour_and_attributes,uint8_t alpha_and_extras)
{
  findFontStructures(font_address);
  
  if (!b) return;
  // Default to allowing 24 rows above and 6 rows below for underhangs
  if (!b->baseline_row) b->baseline_row=24;

  // XXX - Code points are in numerical order, so speed this up with
  // a binary search.
  for(i = 0; i < point_list_length; i += 5)
    {
      POKE(0xD020U,(PEEK(0xD020U)&0xf)+1);

      lcopy(point_list+i,(long)&the_code_point,2);
      if (the_code_point!=code_point) continue;

      // We have the glyph, so dig out the information on it.
      map_pos=0; lcopy(point_list+i+2,(long)&map_pos,3);

      rows_above=lpeek(map+map_pos);
      rows_below=lpeek(map+map_pos+1);
      bytes_per_row=lpeek(map+map_pos+2);

      // If glyph is 0 pixels wide, nothing to do.
      if (!bytes_per_row) continue;
      // If glyph would overrun the buffer, don't draw it.
      if ((bytes_per_row+b->columns_used)>99) continue;
      bytes_per_row=bytes_per_row << 1;

      // Skip header
      map_pos+=3;

      // Remember how many pixels are trimmed from the last glyph,
      // so that we can update the count of trimmed pixels
      trim_pixels=lpeek(map+map_pos+bytes_per_row-1)>>5;

      // Work out first address of screen and colour RAM

      // First, work out the address of the row
      screen=b->screen_ram; colour=b->colour_ram;
      for(y=rows_above;y<b->baseline_row;y++) {
	screen+=200;
	colour+=200;
      }
      // then advance to the first unused column
      screen+=b->columns_used;
      screen+=b->columns_used;
      colour+=b->columns_used;
      colour+=b->columns_used;

      // Now we need to copy each row of data to the screen RAM and colour RAM buffers
      for(y=0;y<(rows_above+rows_below);y++) {
	// Copy screen data
	lcopy((long)map+map_pos,(int)screen,bytes_per_row);
	// Fill colour RAM.
	// Bit 5 of low byte is alpha blending flag.  This is true for font glyphs, and false
	// for graphics tiles.
	// Then bottom 4 bits of high byte is foreground colour, and the upper 4 bits are VIC-III
	// attributes (blink, underline etc)
	// This requires 0x20 for the first byte, to enable alpha blending
	lpoke(colour,alpha_and_extras);
	lpoke(colour+1,colour_and_attributes);
	if (bytes_per_row>2) {
	  lpoke(colour+2,alpha_and_extras);
	  lpoke(colour+3,colour_and_attributes);
	}
	// Then use DMA to copy the pair of bytes out
	// (DMA timing is a bit funny, hence we need to have the two copies setup above
	//  for this to work reliably).
	if (bytes_per_row>4) lcopy((long)colour,(long)colour+2,bytes_per_row-2);

	map_pos+=bytes_per_row;
	screen+=200;
	colour+=200;
      }
      
      break;
    }
  if (i == point_list_length) {
    // Unknown Glyph, so draw a box with the code point number in it?    
  }
  
}

void patchFont(uint32_t font_address)
{
  findFontStructures(font_address);

  // Patch tile_array_start
  tile_array_start+=font_address;
  lcopy((long)&tile_array_start,font_address+0x84,3);
  
  tile_array_start /= 0x40;

  for(i = 0; i < point_list_length; i += 5)
    {
      map_pos=0; lcopy(point_list+i+2,(long)&map_pos,3);
      tile = map+map_pos;
      lcopy((long)&tile,point_list+i+2,3);
      cards = tile + 4;
      
      rows_above=lpeek(tile);
      rows_below=lpeek(tile+1);
      bytes_per_row=lpeek(tile+2);
      glyph_size = (int)(rows_above+rows_below)*bytes_per_row;
      for (j = 0; j < glyph_size; ++j)
        {
	  lcopy(cards,(long)&card,2);
	  card+=tile_array_start;
	  lcopy((long)&card,cards,2);
	  cards += 2;
        }
      
    }
}
