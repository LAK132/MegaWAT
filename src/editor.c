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

// XXX For now, have a fixed 100x30 buffer for text
unsigned int editor_buffer[30][100];

char maxlen = 80;
char key = 0;
char mod = 0;

int xx;
unsigned char h;

extern uint8_t x,y;

render_buffer_t buffer, scratch;

void editor_insert_line(unsigned char before)
{
  // Insert a new blank line before line #before
}

void editor_initialise(void)
{
  for(y=0;y<30;y++) {
    lfill((long)&editor_buffer[y][0],0x00,100*2);
    text_line_rows[y]=0;
  }
  text_line_count=0;

}

void editor_stash_line(unsigned char line_num)
{
  // Stash the line encoded in scratch buffer into the specified line
  
}

void editor_fetch_line(unsigned char line_num)
{
  // Fetch the specified line, and pre-render it into scratch
}

void editor_show_cursor(void)
{
  // Put cursor in initial position
  xx=6; y=30;
  POKE(0xD000,xx & 0xFF);
  POKE(0xD001,y);
  if (xx&0x100) POKE(0xD010U,0x01); else POKE(0xD010U,0);
  if (xx&0x200) POKE(0xD05FU,0x01); else POKE(0xD05FU,0);

}

void editor_update_cursor(void)
{
  // Work out where cursor should be
  xx=6; // Fudge factor
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
  renderGlyph(ASSET_RAM,code_point,&scratch,text_colour,ATTRIB_ALPHA_BLEND,cursor_col);

  // Check if this code point grew the height of the line.
  // If so, push everything else down before pasting
  
  buffer.rows_used=current_row;
  outputLineToRenderBuffer(&scratch,&buffer);
  next_row=buffer.rows_used;
  cursor_col++;
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

		  // Fill in bottom of screen
		  
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

	  editor_update_cursor();
    
	  {
	    int i;
	    for(i=0;i<25000;i++) continue;
	  }
	} else {
	// Toggle cursor on/off quickly
	POKE(0xD015U,(PEEK(0xD015U) ^ 0x01) & 0x0f);
      }
    }
  
}
