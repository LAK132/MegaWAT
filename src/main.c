/*
  A simple slide presentation program for the MEGA65.
  Written by Lucas "LAK132" Kleiss and Paul Gardner-Stephen
*/

#include "main.h"

#ifdef __MEGA65__
void main(void)
#else // PC
void megamain()
#endif
{
    m65_io_enable();

    // Turn off write protection so we can use more RAM
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
