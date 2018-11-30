#include "megaint.h"
#include "memorymap.h"
#include "globals.h"
#include "f65.h"

void videoDisableVicIIHotReg(void);
void videoSetSlideMode(void);
void videoSetActiveRenderBuffer(uint8_t bufferId);
void videoSetActiveGraphicsBuffer(uint8_t bufferId);