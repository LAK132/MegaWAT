#include "../memory.h"
#include <assert.h>

#define RAM_SIZE 0x60000
#define CRAM_SIZE 0x8000
#define CRAM_BASE 0xFF80000

uint8_t m65_ram[RAM_SIZE];   // 0x0000000 -> 0x005FFFF
uint8_t m65_cram[CRAM_SIZE]; // 0xFF80000 -> 0xFF87FFF

void m65_io_enable(void)
{
    // Gate C65 IO enable
    POKE(0xD02FU, 0x47);
    POKE(0xD02FU, 0x53);
    // Force to full speed
    POKE(0, 65);
}

uint8_t lpeek(uint32_t address)
{
    assert((address < sizeof(m65_ram) || address >= CRAM_BASE) && "Out of range");
    return address < sizeof(m65_ram)
        ? m65_ram[address]
        : m65_cram[address - CRAM_BASE];
}

void lpoke(uint32_t address, uint8_t value)
{
    assert((address < sizeof(m65_ram) || address >= CRAM_BASE) && "Out of range");
    if (address < sizeof(m65_ram))
        m65_ram[address] = value;
    else
        m65_cram[address - CRAM_BASE] = value;
}

void lcopy(uint32_t src, uint32_t dst, uint16_t count)
{
    for (uint16_t c = 0; c < count; ++c)
        lpoke(dst + c, lpeek(src + c));
}

uint8_t copy_buffer[0x100];

void lcopy_safe(uint32_t src, uint32_t dst, uint16_t count)
{
    assert(count < sizeof(copy_buffer) && "Out of range");
    for (uint8_t c = 0; c < count; ++c)
        copy_buffer[c] = lpeek(src + c);
    for (uint8_t c = 0; c < count; ++c)
        lpoke(dst + c, copy_buffer[c]);
}

void lfill(uint32_t dst, uint8_t value, uint16_t count)
{
    for (uint16_t c = 0; c < count; ++c)
        lpoke(dst + c, value);
}