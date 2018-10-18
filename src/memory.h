
#include <stdint.h>

void m65_io_enable(void);
uint8_t lpeek(uint32_t address);
void lpoke(uint32_t address, uint8_t value);
void lcopy(uint32_t source_address, uint32_t destination_address, uint16_t count);
void lcopy_safe(uint32_t src, uint32_t dst, uint16_t count);
void lfill(uint32_t destination_address, uint8_t value, uint16_t count);
#define POKE(X, Y) (*(uint8_t *)(X)) = Y
#define PEEK(X) (*(uint8_t *)(X))
