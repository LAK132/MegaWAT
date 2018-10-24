#include "../memory.h"
#include <stdint.h>
#include <assert.h>
#include <lak/utils/ldebug.h>

#define RAM_SIZE 0xFFF0000

uint8_t m65_ram[RAM_SIZE];

void m65_io_enable(void)
{
    // Gate C65 IO enable
    POKE(0xD02FU, 0x47);
    POKE(0xD02FU, 0x53);
    // Force to full speed
    POKE(0, 65);
}

uint8_t lpeek(ptr_t address)
{
    return (longptr_t)address < sizeof(m65_ram)
        ? m65_ram[(longptr_t)address]
        : *(charptr_t)address;
}

void lpoke(ptr_t address, uint8_t value)
{
    if ((longptr_t)address < sizeof(m65_ram))
        m65_ram[(longptr_t)address] = value;
    else
        *(charptr_t)address = value;
}

void lcopy(ptr_t src, ptr_t dst, uint16_t count)
{
    for (uint16_t c = 0; c < count; ++c)
        lpoke(dst + c, lpeek(src + c));
}

uint8_t copy_buffer[0x100];

void lcopy_safe(ptr_t src, ptr_t dst, uint16_t count)
{
    assert(count < sizeof(copy_buffer) && "Out of range");
    for (uint8_t c = 0; c < count; ++c)
        copy_buffer[c] = lpeek(src + c);
    for (uint8_t c = 0; c < count; ++c)
        lpoke(dst + c, copy_buffer[c]);
}

void lfill(ptr_t dst, uint8_t value, uint16_t count)
{
    for (uint16_t c = 0; c < count; ++c)
        lpoke(dst + c, value);
}