/*
    Code to manipulate video modes.
*/

#include "videomodes.h"
#include "main.h"

void videoSetSlideMode(void)
{
    // Enable VicII hot registers
    ENABLE_HOT_REGISTERS(); // 0xD05D

    /* 800x600 with 800x480 active area.
     16 bit text mode with 100 character virtual line length.
    */

    // 50Hz PAL video mode for 576p
    POKE(0xD06fU,0x00);

    // 16-bit text mode, 640H sprites, alpha blender, 50MHz CPU, full-colour for chars >$FF
    POKE(0xd054U,0xD5);

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

    // Minimum side borders for 720px mode
    POKE(0xD05CU, 40-2); // (800 - 720 ) / 2 = 40
    POKE(0xD05DU, PEEK(0xD05DU) & 0x80);

    // Set H640 and V400 and enable extended attributes and 8-bit colour values
    POKE(0xd031,0xa8);

    // Update hot registers
    POKE(0xd011U,0x1b);

    // The following must happen AFTER touching $D011
    // Touching $D011 etc will require them to be set again.

    // 100 characters per line x2 bytes = 200 bytes per row
    POKE(0xd058U,200);
    POKE(0xd059U,0);
    // And make sure we render 100 characters per line
    POKE(0xd05EU,100);

    // Adjust top and bottom borders
    POKE(0xd048,0x52);
    POKE(0xd04A,0x32);
    POKE(0xd04B,0x02);

    // Place character generator adjacent to freshly moved top border
    POKE(0xd04E,0x52);

    // Disable VicII hot registers
    DISABLE_HOT_REGISTERS(); // 0xD05D

    // Align left edge of text appropriately
    POKE(0xD04CU, 40); // (800 - 720 ) / 2 = 40
    
    // Disable maskable interrupts (MEGA+SHIFT)
    __asm__("sei");

    lfill(SLIDE0_SCREEN_RAM, 0, SLIDE_SIZE);
    lfill(SLIDE1_SCREEN_RAM, 0, SLIDE_SIZE);
    lfill(SLIDE0_COLOUR_RAM, 1, SLIDE_SIZE);
    lfill(SLIDE1_COLOUR_RAM, 1, SLIDE_SIZE);

    char_size = sizeof(uint16_t);
    // screen_height = 60;
    screen_width = 100 * char_size;     // 200
    screen_size = 60 * screen_width;    // 12000

    screen_rbuffer.screen_ram = SLIDE0_SCREEN_RAM;
    screen_rbuffer.colour_ram = SLIDE0_COLOUR_RAM;
    screen_rbuffer.screen_size = screen_size;

    scratch_rbuffer.screen_ram = SCRATCH_SCREEN_RAM;
    scratch_rbuffer.colour_ram = SCRATCH_COLOUR_RAM;
    scratch_rbuffer.screen_size = SCRATCH_SIZE;
}

void videoSetActiveRenderBuffer(uint8_t bufferId)
{
    // Only buffers 0 and 1 are displayable,
    // but allow buffer 2 to be used for
    // double buffering (which will copy it later)
    if (bufferId == 1)
    {
        screen_rbuffer.screen_ram = SLIDE1_SCREEN_RAM;
        screen_rbuffer.colour_ram = SLIDE1_COLOUR_RAM;
    }
    else if (bufferId == 2)
    {
        screen_rbuffer.screen_ram = SLIDE2_SCREEN_RAM;
        screen_rbuffer.colour_ram = SLIDE2_COLOUR_RAM;
    }
    else
    {
        screen_rbuffer.screen_ram = SLIDE0_SCREEN_RAM;
        screen_rbuffer.colour_ram = SLIDE0_COLOUR_RAM;
    }
}

void videoSetActiveGraphicsBuffer(uint8_t bufferId)
{
    if (bufferId == 1)
    {
        // Change the screen address pointer
        lpoke(0xffd3060U,(SLIDE1_SCREEN_RAM>>0U)&0xff);
        lpoke(0xffd3061U,(SLIDE1_SCREEN_RAM>>8U)&0xff);
        lpoke(0xffd3062U,(SLIDE1_SCREEN_RAM>>16U)&0xff);
        // change the colour addresss pointer
        lpoke(0xffd3064U,(SLIDE1_COLOUR_RAM>>0U)&0xff);
        lpoke(0xffd3065U,(SLIDE1_COLOUR_RAM>>8U)&0xff);
    }
    else
    {
        // Change the screen address pointer
        lpoke(0xffd3060U,(SLIDE0_SCREEN_RAM>>0U)&0xff);
        lpoke(0xffd3061U,(SLIDE0_SCREEN_RAM>>8U)&0xff);
        lpoke(0xffd3062U,(SLIDE0_SCREEN_RAM>>16U)&0xff);
        // change the colour addresss pointer
        lpoke(0xffd3064U,(SLIDE0_COLOUR_RAM>>0U)&0xff);
        lpoke(0xffd3065U,(SLIDE0_COLOUR_RAM>>8U)&0xff);
    }
}
