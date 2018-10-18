
#include <stdint.h>

#ifdef __CC65__
#define __MEGA65__
#endif

void m65_io_enable(void);
uint8_t lpeek(uint32_t address);
void lpoke(uint32_t address, uint8_t value);
void lcopy(uint32_t source_address, uint32_t destination_address, uint16_t count);
void lcopy_safe(uint32_t src, uint32_t dst, uint16_t count);
void lfill(uint32_t destination_address, uint8_t value, uint16_t count);

#ifdef __MEGA65__
#define POKE(X, Y) (*(uint8_t *)(X)) = Y
#define PEEK(X) (*(uint8_t *)(X))
#else // PC
#define POKE(X, Y) lpoke((uint32_t)(X & 0xFFFF), Y)
#define PEEK(X) lpeek((uint32_t)(X & 0xFFFF))
#endif