#include "main.h"

unsigned char text_colour=1;
unsigned char cursor_col=0;
// Physical rows on the slide (as compared to the lines of text that
// produce them).
unsigned char current_row=0;
unsigned char next_row=0;
// Which line of text we are on
unsigned char text_line=0;
// Rows where each line of text is drawn
unsigned char text_line_rows[30];
unsigned char text_line_count=1;

// XXX For now, have a fixed 128x30 buffer for text
// (We allow 128 instead of 100, so that we can have a number of font and attribute/colour changes
// in there.)
#define EDITOR_LINE_LEN 128
#define EDITOR_MAX_LINES 16
unsigned int editor_buffer[EDITOR_MAX_LINES][EDITOR_LINE_LEN];

char maxlen = 80;
unsigned int key = 0;
char mod = 0;

int xx;
unsigned char h;

extern uint8_t x,y;

render_buffer_t buffer, scratch;

uint8_t cursor_toggle = 0;

void editor_insert_line(unsigned char before)
{
  // Insert a new blank line before line #before
}

void editor_initialise(void)
{
  for(y=0;y<EDITOR_MAX_LINES;y++) {
    lfill((long)&editor_buffer[y][0],0x00,EDITOR_LINE_LEN*2);
    text_line_rows[y]=0;
  }
  text_line_count=0;

}

void editor_stash_line(unsigned char line_num)
{
  // Stash the line encoded in scratch buffer into the specified line

  // Setup default font and attributes, so that we can notice when
  // switching.
  font_id=0;
  text_colour=1;
  
  y=0;
  for(x=0;x<scratch.glyph_count;x++) {
    // Make sure we don't overflow
    if (y>=(EDITOR_LINE_LEN-2)) break;
    
    // Encode any required colour/attribute/font changes
    if (scratch.glyphs[x].colour_and_attributes!=text_colour)
      {
	// Colour change, so write 0xe0XX, where XX is the colour
	editor_buffer[line_num][y++]=0xe000+scratch.glyphs[x].colour_and_attributes;
	text_colour=scratch.glyphs[x].colour_and_attributes;
      }
    if (scratch.glyphs[x].font_id!=font_id)
      {
	editor_buffer[line_num][y++]=scratch.glyphs[x].font_id;
	font_id=scratch.glyphs[x].font_id;
      }

    // Write glyph
    editor_buffer[line_num][y++]=scratch.glyphs[x].code_point;
  }
  // Fill remainder of line with end of line markers
  for(;y<EDITOR_LINE_LEN;y++)
    editor_buffer[line_num][y]=0;
}

void editor_fetch_line(unsigned char line_num)
{
  // Fetch the specified line, and pre-render it into scratch
  clearRenderBuffer(&scratch);
  for(h=0;editor_buffer[line_num][h]&&(h<EDITOR_LINE_LEN);h++)
    if ((editor_buffer[line_num][h]<0xe000)
	||(editor_buffer[line_num][h]>=0xf800))
      renderGlyph(ASSET_RAM, // 
		  editor_buffer[line_num][h],
		  &scratch,
		  text_colour,
		  ATTRIB_ALPHA_BLEND,
		  0xFF   // Append to end of line
		  );
    else {
      // Reserved code points are used to record colour and other attribute changes
      switch (editor_buffer[line_num][h]&0xff00)
	{
	case 0xe000:
	  // Set colour and attributes
	  text_colour=editor_buffer[line_num][h]&0xff;
	  break;
	case 0xe100:
	  // Set font id
	  font_id=editor_buffer[line_num][h]&0xff;
	  // XXX - Set active font
	  break;
	default:
	  break;
	}
    }
}

void editor_show_cursor(void)
{
  // Put cursor in initial position
  xx=5; y=30;
  POKE(0xD000,xx & 0xFF);
  POKE(0xD001,y);
  if (xx&0x100) POKE(0xD010U,0x01); else POKE(0xD010U,0);
  if (xx&0x200) POKE(0xD05FU,0x01); else POKE(0xD05FU,0);

}

void editor_update_cursor(void)
{
  // Work out where cursor should be
  xx=5; // Fudge factor
  for(x=0;x<cursor_col;x++)
    xx+=scratch.glyphs[x].columns*8-scratch.glyphs[x].trim_pixels;
  // Work out cursor height
  h=8*(scratch.max_above+scratch.max_below);
  if (h<8) h=8;
  y=30;
  // Set extended Y height to match required height.
  POKE(0xD056,h);
  // Make cursor be text colour (will alternate to another colour as well)
  POKE(0xD027U,text_colour);
  // Move sprite to there
  POKE(0xD000,xx & 0xFF);
  POKE(0xD001,y);
  if (xx&0x100) POKE(0xD010U,0x01); else POKE(0xD010U,0);
  if (xx&0x200) POKE(0xD05FU,0x01); else POKE(0xD05FU,0);
    
}

void editor_insert_codepoint(unsigned int code_point)
{
  // Natural key -- insert here
  h=scratch.glyph_count;
  renderGlyph(ASSET_RAM,code_point,&scratch,text_colour,ATTRIB_ALPHA_BLEND,cursor_col);

  // Check if this code point grew the height of the line.
  // If so, push everything else down before pasting
  
  buffer.rows_used=current_row;
  outputLineToRenderBuffer(&scratch,&buffer);
  next_row=buffer.rows_used;
  // Only advance cursor if the glyph was actually rendered
  if (scratch.glyph_count>h) cursor_col++;
  if (cursor_col>scratch.glyph_count) cursor_col=scratch.glyphs;
}

void editor_process_special_key(uint8_t key)
{
  switch(key) {
    // Commodore colour selection keys
  case 0x05: text_colour=0; break;
  case 0x1c: text_colour=1; break;
  case 0x9f: text_colour=2; break;
  case 0x9c: text_colour=3; break;
  case 0x1e: text_colour=4; break;
  case 0x1f: text_colour=5; break;
  case 0x9e: text_colour=6; break;
  case 0x81: text_colour=7; break;
    
    // Backspace (with CONTROL for DEL?)
  case 0x14:
    if (cursor_col) {
      deleteGlyph(&scratch,cursor_col-1);
      buffer.rows_used=current_row;
      outputLineToRenderBuffer(&scratch,&buffer);
      if (buffer.rows_used<next_row) {
	// Deleting a character reduced the row height,
	// So shuffle up the rows below, and fill in the
	// bottom of the screen.
	
	// Copy rows up
	lcopy(buffer.screen_ram+next_row*200,
	      buffer.screen_ram+buffer.rows_used*200,
	      (60-next_row)*200);
	
	// XXX Fill in bottom of screen
	
      }
      next_row=buffer.rows_used;
      cursor_col--;
    } else {
      // Backspace from start of line
      // XXX - Should combine remainder of line with previous line (if it will fit)
      // XXX - Shuffle up the rest of the screen,
      // XXX - Select the previous line
      // XXX - Re-render the previous line
      //       (And shuffle everything down in the process if this line grew taller)
    }
    break;
    
    // Cursor navigation within a line
  case 0x9d:
    if (cursor_col) cursor_col--;
    break;
  case 0x1d:
    if (cursor_col<scratch.glyph_count) cursor_col++;
    break;
    
    // Cursor navigation between lines
    // Here we adjust which line we are editing,
    // Fix cursor position if it would be beyond the end of the line
  case 0x0d: // RETURN
    // Break line into two
    editor_insert_line(text_line+1);
    cursor_col=0;
  case 0x11: // Cursor down
    editor_stash_line(text_line);
    if (text_line<(text_line_count-1)) text_line++;
    current_row=text_line_rows[text_line];
    editor_fetch_line(text_line);
    break;
  case 0x91: // Cursor up
    editor_stash_line(text_line);
    if (text_line) text_line--;
    current_row=text_line_rows[text_line];
    editor_fetch_line(text_line);	      
    break;
  default:
    break;
  }

}

void editor_poll_keyboard(void)
{
  while (key != KEY_ESC)
    {
      mod = READ_MOD();
      key = READ_KEY();
      if (key)
	{
	  while (READ_KEY())
	    {
	      unsigned int i;
	      READ_KEY() = 1;
	      mod |= READ_MOD();
	    }
	  
	  if (key>=' '&&key<=0x7e) {
	    editor_insert_codepoint(key);
	  } else {
	    editor_process_special_key(key);
	  }

	  editor_update_cursor();
    
	  {
	    int i;
	    for(i=0;i<25000;i++) continue;
	  }
	} else {
	if (PEEK(0xD012U)>0xF8)
	  {
	    cursor_toggle++;
	    if (!cursor_toggle)
	      // Toggle cursor on/off quickly
	      POKE(0xD015U,(PEEK(0xD015U) ^ 0x01) & 0x0f);
	  }
      }
    }
  
}
