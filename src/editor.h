#include "f65.h"
#include "globals.h"
#include "memory.h"
#include "screen.h"
#include "videomodes.h"
#include "serial.h"
#include "stdio.h"
#include "fileio.h"

#ifndef EDITOR_H
#define EDITOR_H

void editor_show_cursor(void);
void editor_poll_keyboard(void);
void editor_initialise(void);

#endif // EDITOR_H