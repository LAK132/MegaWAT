
#include "megaint.h"

void m65_io_enable(void);
uint8_t lpeek(ptr_t address);
void lpoke(ptr_t address, uint8_t value);
void lcopy(ptr_t source_address, ptr_t destination_address, uint16_t count);
void lcopy_safe(ptr_t src, ptr_t dst, uint16_t count);
void lfill(ptr_t destination_address, uint8_t value, uint16_t count);

#ifdef __MEGA65__
#define POKE(X, Y) (*(charptr_t)(X)) = Y
#define PEEK(X) (*(charptr_t)(X))
#else // PC
#define POKE(X, Y) lpoke((ptr_t)(shortptr_t)X, Y)
#define PEEK(X) lpeek((ptr_t)(shortptr_t)X)
#endif