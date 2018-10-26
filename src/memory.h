
#include "megaint.h"

void m65_io_enable(void);

// #define __MEGA65__

#ifdef __MEGA65__
extern uint32_t fast_memory_address;
extern uint8_t fast_memory_value;
// Use MEGA65 32-bit zero-page indirect instead of constructing a DMA job
// The result is several times faster, especially for lpoke, where we avoid
// argument passing
#define FAST_LPOKE
#endif

uint8_t lpeek(ptr_t address);
void lpoke(ptr_t address, uint8_t value);

#ifdef FAST_LPOKE
lpoke_asm();
lpeek_asm();
// // Macros to wrap the fast implementations of lpoke and lpeek
// #define lpoke(A, V) { fast_memory_address = A; fast_memory_value = V; lpoke_asm(); }
// #define lpeek(A) lpeek_fast(A)
#endif // FAST_LPOKE

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