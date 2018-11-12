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

#ifdef __MEGA65__
void main(void)
#else // PC
void megamain()
#endif
{
    m65_io_enable();

    // Turn off write protection so we can use more RAM
    toggle_write_protection();

#if 0
    // Draw alpha gradient for testing
    for (x = 0; x < 255; ++x)
        for (y = 0; y < 8; ++y)
            POKE(0xE000U + (x & 7) + ((x / 8) * 64U) + (y * 8), x);

    for (x = 0; x < 100; ++x)
    {
        lpoke(SLIDE0_SCREEN_RAM + 220U + x * 2, (0xe000U / 0x40 + x) & 0xff);
        lpoke(SLIDE0_SCREEN_RAM + 220U + x * 2 + 1, (0xe000U / 0x40 + x) >> 8);
        lpoke(SLIDE0_COLOUR_RAM + 220U + x * 2, 0x20); // 0x20 for alpha
        lpoke(SLIDE0_COLOUR_RAM + 220U + x * 2 + 1, 0x2);
    }

    x = 90;
    lpoke(SLIDE0_SCREEN_RAM + 200U + x * 2, 0xff);
    lpoke(SLIDE0_SCREEN_RAM + 200U + x * 2 + 1, 0xff);
    lpoke(SLIDE0_SCREEN_RAM + 0U + x * 2, 0xff);
    lpoke(SLIDE0_SCREEN_RAM + 0U + x * 2 + 1, 0xff);

    while (1)
        POKE(0xd020U, PEEK(0xd012U));
#endif
    editor_initialise();

    // Then patch the pointers in the font(s) to be correct
    current_font = ASSET_RAM;
    patchFonts();
    setFont(0);

    editor_show_cursor();

    #ifdef __MEGA65__
    for (;;)
        // continue;
        editor_poll_keyboard();
    #endif
}
