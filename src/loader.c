#include "main.h"

extern const unsigned short splashlogo;

unsigned short n;
unsigned char x,y;
unsigned short glyph;

void main(void)
{
    m65_io_enable();

    // Go back to upper case
    POKE(0xD018,0x14);

    // Copy palette into place
    lcopy(splashlogo,0xFFD3100U,0x300);
    
    // Copy splash logo to the top of RAM
    // (can be upto 32KB)
    lcopy(splashlogo+0x300,0x58000U,32767);

    // Copy screen to 0x57800 to make 16-bit version of screen
    // Do similar for colour RAM
    for(n=0;n<1000;n++) {
      // Screen RAM
      lpoke(0x57800U+n*2,PEEK(0x0400+n));
      lpoke(0x57800U+n*2+1,0);

      // Colour RAM
      lpoke(0xFF80800+n*2+0,0);
      lpoke(0xFF80800+n*2+1,PEEK(0xD800U+n));
    }     

    // Draw logo on the screen.
    // 240 x 128 = 30 x 16 rows
    for(x=0;x<30;x++)
      for(y=0;y<16;y++) {
	glyph=(0x58000U/0x40)+x+y*30;
	lpoke(0x57800U+0+(5*2)+(4*40*2)+x*2+y*(40*2),glyph&0xff);
	lpoke(0x57800U+1+(5*2)+(4*40*2)+x*2+y*(40*2),glyph>>8);
      }
    
    // Move start of screen address and colour RAM and set 16-bit text mode, set screen line length to 40*2=80 bytes
    POKE(0xD054U,0xC5);
    POKE(0xD058U,40*2);
    POKE(0xD060U,0x00); POKE(0xD061U,0x78); POKE(0xD062U,0x05);  // screen at 0x57800
    POKE(0xD064U,0x00); POKE(0xD065U,0x08); // colour RAM at offset 0x0800

    // Then pretend to load for now
    while(1) {
      POKE(0xD020U,0x0e);
      for(n=0;n<12000;n++) continue;
      POKE(0xD020U,0x01);
      for(x=0;x<25;x++) continue;
    }
    
    return;
}
