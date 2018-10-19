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

    videoSetSlideMode();
    videoSetActiveSlideBuffer(0);

#if 0
    // Draw alpha gradient for testing
    for (x = 0; x < 255; x++)
        for (y = 0; y < 8; y++)
            POKE(0xE000U + (x & 7) + ((x / 8) * 64U) + (y * 8), x);

    for (x = 0; x < 100; x++)
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

    // Copy bundled font into the asset area of memory
    lcopy(font_file, ASSET_RAM, font_file_size);
    // Then patch the pointers in the font to be correct
    patchFont(ASSET_RAM);

    // Create a render buffer that points to the default active screen
    screen_rbuffer.screen_ram = SLIDE0_SCREEN_RAM;
    screen_rbuffer.colour_ram = SLIDE0_COLOUR_RAM;

    scratch_rbuffer.screen_ram = SCRATCH_SCREEN_RAM;
    scratch_rbuffer.colour_ram = SCRATCH_COLOUR_RAM;

    // Make sure they are clear
    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();

    editor_initialise();
    editor_show_cursor();

    #ifdef __MEGA65__
    for (;;)
        // continue;
        editor_poll_keyboard();
    #endif
}
