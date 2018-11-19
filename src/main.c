/*
  A simple slide presentation program for the MEGA65.

*/

#include "main.h"

#ifdef __MEGA65__
void main(void)
#else // PC
void megamain()
#endif
{
    m65_io_enable();
    // disable keyboard interupt
    // POKE(0x0314, 0x81);
    // disable all maskable interupt
    __asm__("sei");

    // Turn off write protection so we can use more RAM
    toggle_write_protection();

    // while (1)
    //     POKE(0xd020U, PEEK(0xd020U)+1);

    // Then patch the pointers in the font(s) to be correct
    current_font = ASSET_RAM;
    patchFonts();
    setFont(0);

    editor_initialise();

    editor_show_cursor();

    #ifdef __MEGA65__
    for (;;)
        // continue;
        editor_poll_keyboard();
    #endif
}
