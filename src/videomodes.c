/*
    Code to manipulate video modes.
*/

#include "videomodes.h"
#include "main.h"

void videoSetSlideMode(void)
{
    /* 800x600 with 800x480 active area.
     16 bit text mode with 100 character virtual line length.
    */

    // 16-bit text mode, 640H sprites, alpha blender and 50MHz CPU, horizontal smoothing (require to deal with artifacts)
    POKE(0xd054U,0xDD);

    // Extended height mode for sprite 0 (cursor)
    POKE(0xD055,1);

    // Set Sprite 0 to point to $0400, where we draw a nice cursor sprite (4 pixels wide solid)
    POKE(2040,0x400/0x40);
    POKE(0x0400,0xf0); POKE(0x0401,0x00); POKE(0x0402,0x00);
    lcopy(0x0400,0x0403,768-3);

    // Cursor goes behind text
    POKE(0xD01BU,1);

    // Cursor is white (toggles black/white)
    POKE(0xD027U,1);

    // Enable sprite 0 (cursor)
    POKE(0xD015U,0x01);

    // No side borders
    POKE(0xd05cU,0);
    POKE(0xd05dU,0);

    // // Set H640 and V400 and enable extended attributes and 8-bit colour values
    POKE(0xd031,0xa8);

    // // Update hot registers
    POKE(0xd011U,0x1b);

    // The following must happen AFTER touching $D011
    // Touching $D011 etc will require them to be set again.

    // 100 characters per line x2 bytes = 200 bytes per row
    POKE(0xd058U,200);
    POKE(0xd059U,0);

    // Adjust top and bottom borders
    POKE(0xd048,0x52);
    POKE(0xd04A,0x32);
    POKE(0xd04B,0x02);

    // Place character generator adjacent to freshly moved top border
    POKE(0xd04E,0x52);


    lfill(SLIDE0_SCREEN_RAM, 0, SLIDE_SIZE);
    lfill(SLIDE1_SCREEN_RAM, 0, SLIDE_SIZE);
    lfill(SLIDE0_COLOUR_RAM, 1, SLIDE_SIZE);
    lfill(SLIDE1_COLOUR_RAM, 1, SLIDE_SIZE);

    screen_address = SLIDE0_SCREEN_RAM;
    colour_address = SLIDE0_COLOUR_RAM;
    char_size = sizeof(uint16_t);
    // screen_height = 60;
    screen_width = 100 * char_size;     // 200
    screen_size = 60 * screen_width;    // 12000
}

void videoSetActiveSlideBuffer(uint8_t bufferId)
{
    screen_address=SLIDE0_SCREEN_RAM;
    colour_address=SLIDE0_COLOUR_RAM;

    // Reject invalid slide buffer numbers
    //   if (bufferId>2) return;    SEE BELOW

    // Actually, SLIDE2_COLOUR_RAM is not in colour RAM,
    // so it cannot be set active, so only buffers 0 and 1
    // are displayable.
    if (bufferId>1) return;

    if (bufferId==1) {
        screen_address=SLIDE1_SCREEN_RAM;
        colour_address=SLIDE1_COLOUR_RAM;
    }

    lpoke(0xffd3060U,(screen_address>>0U)&0xff);
    lpoke(0xffd3061U,(screen_address>>8U)&0xff);
    lpoke(0xffd3062U,(screen_address>>16U)&0xff);

    lpoke(0xffd3064U,(colour_address>>0U)&0xff);
    lpoke(0xffd3065U,(colour_address>>8U)&0xff);

    return;
}
