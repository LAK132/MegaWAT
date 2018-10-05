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

#ifdef __CC65__
void main(void)
#else
int main(int argc,char **argv)
#endif
{
    // char i = 0;
    char maxlen = 80;
    char key = 0;
    char mod = 0;
    uint32_t sdsize = 0;

#ifdef __CC65__
    mega65_fast();
    setup_screen();
#endif

    // set border and screen colours
    BORDER_COLOUR(COLOUR_BLACK);
    SCREEN_COLOUR(COLOUR_BLACK);

    // Write a line of text
    // write_line("MEGA WAT! 8bit PowerPoint",0);
    // Change the colour of that line of text
    // recolour_last_line(COLOUR_WHITE);

    // for (i = 0; i < 25; ++i)
    //     write_string("Hello", i, i);

    // // Read a line of input, and print it back out
    // {
    //     char line_of_input[80];
    //     unsigned char len;
    //     len=read_line2(line_of_input,80);
    //     if (len) {
    //         write_line(line_of_input,0);
    //         recolour_last_line(COLOUR_YELLOW);
    //     }
    // }

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
                for(i=0;i<25000;++i) mod |= READ_MOD();
            }
            if (key == KEY_LEFT_RIGHT)
            {
                if (mod & MOD_SHIFT) // LEFT
                {
                    MOVE_CURSOR(-1);
                }
                else // RIGHT
                {
                    MOVE_CURSOR(1);
                }
            }
            else if (key == KEY_UP_DOWN)
            {
                if (mod & MOD_SHIFT) // UP
                {
                    MOVE_CURSOR(-80);
                }
                else // DOWN
                {
                    MOVE_CURSOR(80);
                }
            }
            else if (key == KEY_BACKSPACE)
            {
                long diff = ((screen_line_address-SCREEN_ADDRESS)%80);
                long len = -1;
                diff = diff == 0 ? 80 : diff;
                if (mod & MOD_CTRL)
                {
                    char c = PEEK(screen_line_address + len);
                    while ((screen_line_address + len > SCREEN_ADDRESS) && (len > -diff) && (c == 0 || c == ' '))
                        c = PEEK(screen_line_address + (--len));
                    if (c != 0 && c != ' ' && len < -1) ++len;
                }
                lcopy(screen_line_address, screen_line_address+len, SCREEN_ADDRESS+(23*80)-screen_line_address);
                lcopy(screen_line_address+SCREEN_COLOUR_OFFSET,
                    screen_line_address+SCREEN_COLOUR_OFFSET+len, SCREEN_ADDRESS+(23*80)-screen_line_address);
                MOVE_CURSOR(len);
            }
            else if (key == KEY_RETURN)
            {
                long i = SCREEN_ADDRESS+(23*80);
                long diff = 80-((screen_line_address-SCREEN_ADDRESS)%80);
                for (; i > screen_line_address + diff; i -= 80)
                {
                    lcopy(i - 80, i, 80);
                    lcopy(i + SCREEN_COLOUR_OFFSET - 80, i + SCREEN_COLOUR_OFFSET, 80);
                }
                lcopy(screen_line_address, screen_line_address + diff, diff);
                lcopy(screen_line_address + SCREEN_COLOUR_OFFSET,
                    screen_line_address + diff + SCREEN_COLOUR_OFFSET, diff);
                lfill(screen_line_address, ' ', diff);
                lfill(screen_line_address+SCREEN_COLOUR_OFFSET, 1, diff);
                MOVE_CURSOR(diff);
            }
            else WRITE_CHAR(key);
            // WRITE_STRING("Hello");
            SET_CURSOR_ATTRIB(ATTRIB_REVERSE);
        }
    }

    // This program doesn't clean up for return to C64 BASIC,
    // so it finishes in an infinite loop.
#ifdef __CC65__
    while(1) continue;
#else
    return 0;
#endif

}
