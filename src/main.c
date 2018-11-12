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

  //    while (1)
  //      POKE(0xd020U, PEEK(0xd020U)+1);
    
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
