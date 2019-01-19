#include "main.h"

extern const unsigned short splashlogo;

unsigned short n;
unsigned char x,y,i;
unsigned short glyph;

unsigned char palette_save[0x300];

void main(void)
{
    m65_io_enable();

    // Go back to upper case
    POKE(0xD018,0x14);

    // Save palette
    lcopy(0xFFD3100U,palette_save,0x300);
    
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
    for(i=0;i<100;i++) {
      POKE(0xD020U,0x0e);
      for(n=0;n<12000;n++) continue;
      POKE(0xD020U,0x01);
      for(x=0;x<25;x++) continue;
    }
    POKE(0xD020U,0x0e);

    POKE(0xD054U,0x00);
    POKE(0xD058U,40);
    POKE(0xD060U,0x00); POKE(0xD061U,0x04); POKE(0xD062U,0x00);  // screen at 0x57800
    POKE(0xD064U,0x00); POKE(0xD065U,0x00); // colour RAM at offset 0x0800

    // Restore palette, hide splash ready for loading next part
    // Save palette
    lcopy(0xFFD3100U,palette_save,0x300);    
    
    // Then actually load the cracktro
    {
      unsigned char link_routine[66]=
	{
	  0xA9,0x37,  // LDA #$37
	  0x85,0x01,  // STA $01
	  0xA2,0x00,  // LDX #$00
	  0x8E,0x16,0xD0, // STX $D016
	  0x20,0x84,0xFF, // JSR $FF84
	  0x20,0x87,0xFF, // JSR $FF87
	  0x20,0x8A,0xFF, // JSR $FF8A
	  0x20,0x81,0xFF, // JSR $FF81
	  0xA9,0x06, // LDA #$06 (length of file name)
	  0xA2,0x3B, // LDX #<filename
	  0xA0,0x01, // LDY #>filename
	  0x20,0xBD,0xFF,  // JSR $FFBD
	  0xA9,0x01, // LDA #$01
	  0xA6,0xBA, // LDX $BA
	  0xD0,0x02, // BNE to the LDY #$01
	  0xA2,0x08, // LDA #$08 (default to device 8)
	  0xA0,0x01, // LDY #$01
	  0x20,0xBA,0xFF, // JSR $FFBA
	  0xA9,0x00, // LDA #$00
	  0x20,0xD5,0xFF, // JSR $FFD5
	  0xB0,0x03, // BCS to the failure loop
	  0x4C,0x0D,0x08,  // JMP $080D to start loaded program
	  0xEE,0x20,0xD0,0x4C,0x35,0x01,  // If failed, INC $D020 + JMP *-3
	  0x50,0x41,0x52,0x54,0x31,0x2A,0x00   // "PART1*" + 0x00
	};
      lcopy(link_routine,0x0400,66);
      lcopy(link_routine,0x0100,66);
      __asm__("jmp $0100");
    }
    
    
    return;
}
