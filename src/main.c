/*
    A simple example program using CC65 and some simple routines to
    use an 80 column ASCII screen on the MEGA65.

    If you follow some restrictions, it is also possible to make such
    programs compile and run natively on your development system for
    testing.

*/

/*
10 POKE 54800,0
20 A=PEEK(54800)
30 IF A=0 THEN 20
40 B=PEEK(54801)
50 PRINT "KEY "; A; B
60 IF A<>20 THEN 10
*/

// Q 51 01010001
// q 71 01110001
// M 4D 01001101
// m 6D 01101101

// RA 1D 00011101
// LA 9D 10011101
// DA 11 00010001
// UA 91 10010001

#include "main.h"

extern int16_t font_file;
extern int16_t font_file_size;

extern uint8_t x, y;

render_buffer_t buffer, scratch;

unsigned char text_colour=1;
unsigned char cursor_col=0;

char maxlen = 80;
char key = 0;
char mod = 0;

int xx;
unsigned char h;

void main(void)
{
  m65_io_enable();
  
  videoSetSlideMode();
  videoSetActiveSlideBuffer(0);
  
#if 0
  // Draw alpha gradient for testing
  for(x=0;x<255;x++)
    for(y=0;y<8;y++)
      POKE(0xE000U+(x&7)+((x/8)*64U)+(y*8),x);

  for(x=0;x<100;x++) {
    lpoke(SLIDE0_SCREEN_RAM+220U+x*2,(0xe000U/0x40+x)&0xff);
    lpoke(SLIDE0_SCREEN_RAM+220U+x*2+1,(0xe000U/0x40+x)>>8);
    lpoke(SLIDE0_COLOUR_RAM+220U+x*2,0x20); // 0x20 for alpha
    lpoke(SLIDE0_COLOUR_RAM+220U+x*2+1,0x2);
  }

  x=90;
  lpoke(SLIDE0_SCREEN_RAM+200U+x*2,0xff);
  lpoke(SLIDE0_SCREEN_RAM+200U+x*2+1,0xff);
  lpoke(SLIDE0_SCREEN_RAM+0U+x*2,0xff);
  lpoke(SLIDE0_SCREEN_RAM+0U+x*2+1,0xff);

    
  while(1) POKE(0xd020U,PEEK(0xd012U));
#endif
    
  // Copy bundled font into the asset area of memory
  lcopy(font_file + 2, ASSET_RAM, font_file_size - 2);
  // Then patch the pointers in the font to be correct
  patchFont(ASSET_RAM);

  // Create a render buffer that points to the default active screen
  buffer.screen_ram = SLIDE0_SCREEN_RAM;
  buffer.colour_ram = SLIDE0_COLOUR_RAM;

  scratch.screen_ram = SCRATCH_SCREEN_RAM;
  scratch.colour_ram = SCRATCH_COLOUR_RAM;

  // Make sure they are clear
  clearRenderBuffer(&buffer);
  clearRenderBuffer(&scratch);

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
	    // Natural key -- insert here
	    renderGlyph(ASSET_RAM,key,&scratch,text_colour,ATTRIB_ALPHA_BLEND,cursor_col);
	    outputLineToRenderBuffer(&scratch,&buffer);
	    cursor_col++;
	    POKE(0xd020U,scratch.glyph_count&0xf);
	  } else {
	    switch(key) {
	    case 0x05: text_colour=0; break;
	    case 0x1c: text_colour=1; break;
	    case 0x9f: text_colour=2; break;
	    case 0x9c: text_colour=3; break;
	    case 0x1e: text_colour=4; break;
	    case 0x1f: text_colour=5; break;
	    case 0x9e: text_colour=6; break;
	    case 0x81: text_colour=7; break;
	    case 0x14:
	      if (cursor_col) {
		deleteGlyph(&scratch,cursor_col-1);
		outputLineToRenderBuffer(&scratch,&buffer);
		POKE(0xd020U,scratch.glyph_count&0xf);
		cursor_col--;
	      }
	      break;
	    case 0x9d:
	      if (cursor_col) cursor_col--;
	      break;
	    case 0x1d:
	      if (cursor_col<scratch.glyph_count) cursor_col++;
	      break;
	    default:
	      break;
	    }
	    POKE(0xD020,text_colour);
	  }

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
    
    
	  {
	    int i;
	    for(i=0;i<25000;i++) continue;
	  }
	} else {
	// Toggle cursor on/off quickly
	POKE(0xD015U,(PEEK(0xD015U) ^ 0x01) & 0x0f);
      }
    }
    
    // renderTextASCII(ASSET_RAM, "line", &scratch, ATTRIB_BLINK | ATTRIB_UNDERLINE | ATTRIB_ALT_PALETTE | COLOUR_YELLOW, ATTRIB_ALPHA_BLEND);
    // outputLineToRenderBuffer(&scratch, &buffer);
    
#if 0
    cursor_attrib = ATTRIB_ALT_PALETTE | ((ATTRIB_ALPHA_BLEND) << 8);

    writeChar('!');

    cursor_attrib = ATTRIB_ALT_PALETTE | ((ATTRIB_ALPHA_BLEND) << 8);

    writeString(hello_world, sizeof(hello_world));

    setCursor(screen_width);

    writeString(hello_world, wstrlen(hello_world));

    while (key != KEY_ESC)
    {
        mod = READ_MOD();
        key = READ_KEY() & 0x7F;
        if (key)
        {
            while (READ_KEY())
            {
                unsigned int i;
                READ_KEY() = 1;
                for(i = 0; i < 25000; ++i) mod |= READ_MOD();
            }
            if (key == KEY_LEFT_RIGHT)
            {
                if (mod & MOD_SHIFT) // LEFT
                    moveCursor(-1);
                else // RIGHT
                    moveCursor(1);
            }
            else if (key == KEY_UP_DOWN)
            {
                if (mod & MOD_SHIFT) // UP
                {
                    if (mod & MOD_CTRL)
                        scrollUp();
                    else
                        moveCursor(-screen_width);
                }
                else // DOWN
                {
                    if (mod & MOD_CTRL)
                        scrollDown();
                    else
                        moveCursor(screen_width);
                }
            }
            else if (key == KEY_BACKSPACE)
            {
                long diff = cursor_position % screenWidth();
                long len = -char_size;
                diff = diff == 0 ? screenWidth() : diff;
                if (mod & MOD_CTRL)
                {
                    char c = lpeek(cursor_position + screen_address + len);
                    while ((cursor_position + len > 0) && (len > -diff) && (c == 0 || c == ' '))
                    {
                        len -= char_size;
                        c = lpeek(cursor_position + screen_address + (len));
                    }
                    if (c != 0 && c != ' ' && len < -char_size) len += char_size;
                }
                liftCursor();
                lcopy(cursor_position + screen_address, cursor_position + screen_address + len,
                    ((screen_height-1)*screenWidth())-cursor_position);
                lcopy(cursor_position + colour_address, cursor_position + colour_address + len,
                    ((screen_height-1)*screenWidth())-cursor_position);
                // cursor_position += len;
                moveCursor(len/char_size);
            }
            else if (key == KEY_RETURN)
            {
                long i = (screen_height-1) * screenWidth();
                long diff = screenWidth() - (cursor_position % screenWidth());
                for (; i > cursor_position + diff; i -= screenWidth())
                {
                    lcopy(i + screen_address - screenWidth(), i + screen_address, screenWidth());
                    lcopy(i + colour_address - screenWidth(), i + colour_address, screenWidth());
                }
                liftCursor();
                lcopy(cursor_position + screen_address, cursor_position + screen_address + diff, diff);
                lcopy(cursor_position + colour_address, cursor_position + colour_address + diff, diff);
                // lfill(cursor_position + screen_address, ' ', diff);
                // lfill(cursor_position + colour_address, 0x1, diff);
                // cursor_position += diff;
                // for(; diff > 0; diff -= char_size)
                //     writeChar(' ');
                writeChars(' ', diff/char_size);
                // moveCursor(diff);
            }
            else if (mod & MOD_CTRL) // control characters
            {
                lpoke(screen_address, key);
                switch (key)
                {
                    case 'B':
                    case 'b': {
                        char_attrib ^= ATTRIB_BOLD;// << (8 * (char_size - 1));
                    } break;
                    case 'R':
                    case 'r': {
                        char_attrib ^= ATTRIB_REVERSE;// << (8 * (char_size - 1));
                    } break;
                    case 'U':
                    case 'u': {
                        char_attrib ^= ATTRIB_UNDERLINE;// << (8 * (char_size - 1));
                    } break;
                    // case 'C':
                    // case 'c': {
                    //     char_attrib
                    // } break;
                }
                // applyAttrib(1);
            }
            else writeChar(key);
            // WRITE_STRING("Hello");
            // SET_CURSOR_ATTRIB(ATTRIB_REVERSE, cursor_position, colour_address);
        }
    }
#endif

        // This program doesn't clean up for return to C64 BASIC,
        // so it finishes in an infinite loop.
#ifdef __CC65__
    while (1)
        continue;
#else
    return 0;
#endif
}

void scrollDown(void)
{
    int32_t count = (screen_height - 1) * screenWidth();
    liftCursor();
    lcopy(screen_address + screenWidth(), screen_address, count);
    lcopy(colour_address + screenWidth(), colour_address, count);
    lfill(screen_address + count, 0, screenWidth());
    moveCursor(-screen_width);
    applyAttrib(screen_width);
}

void scrollUp(void)
{
    int32_t i = (screen_height - 1) * screenWidth();
    liftCursor();
    for (; i > 0; i -= screenWidth())
    {
        lcopy(screen_address + i - screenWidth(), screen_address + i, screenWidth());
        lcopy(colour_address + i - screenWidth(), colour_address + i, screenWidth());
    }
    lfill(screen_address, 0, screenWidth());
    applyAttrib(screen_width);
    moveCursor(screen_width);
}

// void liftCursor(void)
// {
//     // lpoke(cursor_position + colour_address, char_attrib & 0xFF);
//     // if (char_size > 1)
//     //     lpoke(cursor_position + colour_address + 1, char_attrib >> 8);
// }

void setCursor(int32_t x)
{
    // reset attributes of previous cursor position
    liftCursor();

    // move cursor and scroll if off screen
    cursor_position = x * char_size;
    while (cursor_position < 0)
        scrollUp();
    while (cursor_position > screenSize())
        scrollDown();

    // save the attributes of the character at cursor
    // char_attrib = lpeek(cursor_position + colour_address);
    // if (char_size > 1)
    //     char_attrib |= lpeek(cursor_position + colour_address + 1) << 8;

    // apply the cursor attributes
    lpoke(cursor_position + colour_address, cursor_attrib & 0xFF);
    if (char_size > 1)
        lpoke(cursor_position + colour_address + 1, cursor_attrib >> 8);
}

void writeString(void *str, uint32_t len)
{
    int32_t i;
    while (cursor_position + len >= screenSize())
        scrollDown();
    lcopy((long)str, cursor_position + screen_address, len);
    applyAttrib(len);
    setCursor(cursor_position + len);
}

void writeChar(uint16_t c)
{
    while (cursor_position + char_size >= screenSize())
        scrollDown();
    lpoke(cursor_position + screen_address, c & 0xFF);
    if (char_size > 1)
        lpoke(cursor_position + screen_address + 1, c >> 8);
    applyAttrib(1);
    moveCursor(1);
}

void writeChars(uint16_t c, int32_t len)
{
    int32_t i;
    while (cursor_position + (len * char_size) >= screenSize())
        scrollDown();
    for (i = 0; i < len; ++i)
    {
        lpoke(cursor_position + screen_address + (i * char_size), c & 0xFF);
        if (char_size > 1)
            lpoke(cursor_position + screen_address + 1 + (i * char_size), c >> 8);
    }
    applyAttrib(len);
    moveCursor(len);
}

void applyAttrib(int32_t len)
{
    while (len-- > 0)
    {
        if (char_size > 1)
        {
            lpoke(cursor_position + colour_address + 1 + (len * char_size), char_attrib & 0xFF);
            lpoke(cursor_position + colour_address + (len * char_size), char_attrib >> 8);
        }
        else
            lpoke(cursor_position + colour_address + (len * char_size), char_attrib & 0xFF);
    }
}

uint32_t wstrlen(uint16_t *str)
{
    uint32_t len = 0;
    while (str[len] != 0)
        ++len;
    return len + len; // return number of *bytes*
}
