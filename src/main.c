/*
  A simple slide presentation program for the MEGA65.
  Written by Lucas "LAK132" Kleiss and Paul Gardner-Stephen
*/

#include "main.h"

unsigned char palette[48]={
  0x00,0xff,0xba,0x66,0xbb,0x55,0xd1,0xae,0x9b,0x87,0xdd,0xb5,0xb8,0x0b,0xaa,0x8b,
  0x00,0xff,0x13,0xad,0xf3,0xec,0xe0,0x5f,0x47,0x37,0x39,0xb5,0xb8,0x4f,0xd9,0x8b,
  0x00,0xff,0x62,0xff,0x8b,0x85,0x79,0xc7,0x81,0x00,0x87,0xb5,0xb8,0xca,0xfe,0xb8
};

#ifdef __MEGA65__
void main(void)
#else // PC
void megamain()
#endif
{
    m65_io_enable();

    // Set default palette (cracktro loads its own different one)
    lcopy(&palette[0],0xFFD3100U,16);
    lcopy(&palette[16],0xFFD3200U,16);
    lcopy(&palette[32],0xFFD3300U,16);
    
    // Turn off write protection of ROM area so we can use more RAM
    toggle_write_protection();

    // Then patch the pointers in the font(s) to be correct
    current_font = ASSET_RAM;
    patchFonts();
    setFont(0);

    editor_start();

    #ifdef __MEGA65__
    for (;;)
    #endif
        editor_poll_keyboard();
}
