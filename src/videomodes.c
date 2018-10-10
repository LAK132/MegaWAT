/*
  Code to manipulate video modes.

*/

#include "main.h"

void videoSetSlideMode(void)
{
  /* 800x600 with 800x480 active area.
     16 bit text mode with 100 character virtual line length.
  */

  // 16-bit text mode, 640H sprites, alpha blender and 50MHz CPU
  POKE(0xd054U,0xD5);

  // 100 characters per line
  POKE(0xd058U,100);
  POKE(0xd059U,0);

  // No side borders
  POKE(0xd05cU,0);
  POKE(0xd05dU,0);

  // Update hot registers
  POKE(0xd011U,0x1b);
}

void videoSetActiveSlideBuffer(unsigned bufferId)
{
  unsigned long screen_ram_base=SLIDE0_SCREEN_RAM;
  unsigned long colour_ram_base=SLIDE0_COLOUR_RAM;

  // Reject invalid slide buffer numbers
  //   if (bufferId>2) return;    SEE BELOW

  // Actually, SLIDE2_COLOUR_RAM is not in colour RAM,
  // so it cannot be set active, so only buffers 0 and 1
  // are displayable.
  if (bufferId>1) return;

  if (bufferId==1) {
    screen_ram_base=SLIDE1_SCREEN_RAM;
    colour_ram_base=SLIDE1_COLOUR_RAM;
  }
  
  lpoke(0xffd3060U,(screen_ram_base>>0U)&0xff);
  lpoke(0xffd3061U,(screen_ram_base>>8U)&0xff);
  lpoke(0xffd3062U,(screen_ram_base>>16U)&0xff);

  lpoke(0xffd3064U,(colour_ram_base>>0U)&0xff);
  lpoke(0xffd3065U,(colour_ram_base>>8U)&0xff);

  return;
}
