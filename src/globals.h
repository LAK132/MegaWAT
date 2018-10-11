#include <stdint.h>

extern int8_t char_size;
extern int32_t screen_width;
extern int32_t screen_height;
extern int32_t cursor_position;
extern uint16_t cursor_attrib;
extern uint16_t char_attrib;
extern uint32_t screen_address;
extern uint32_t colour_address;

#define screen_size (screen_size * screen_height)
#define screenWidth() (screen_width * char_size)
#define screenSize() (screenWidth() * screen_height)