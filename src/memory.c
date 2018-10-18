/*
  MEGA65 Memory Access routines that allow access to the full RAM of the MEGA65,
  even though the program is stuck living in the first 64KB of RAM, because CC65
  doesn't (yet) understand how to make multi-bank MEGA65 programs.

*/

#include "memory.h"


struct dmagic_dmalist {
  // Enhanced DMA options
  uint8_t option_0b;
  uint8_t option_80;
  uint8_t source_mb;
  uint8_t option_81;
  uint8_t dest_mb;
  uint8_t end_of_options;

  // F018B format DMA request
  uint8_t command;
  uint16_t count;
  uint16_t source_addr;
  uint8_t source_bank;
  uint16_t dest_addr;
  uint8_t dest_bank;
  uint8_t sub_cmd;  // F018B subcmd
  uint16_t modulo;
};

struct dmagic_dmalist dmalist;
uint8_t dma_byte;

uint8_t copy_buffer[256];

void do_dma(void)
{
  m65_io_enable();

  // Now run DMA job (to and from anywhere, and list is in low 1MB)
  POKE(0xd702U,0);
  POKE(0xd704U,0x00);  // List is in $00xxxxx
  POKE(0xd701U,((uint16_t)&dmalist)>>8);
  POKE(0xd705U,((uint16_t)&dmalist)&0xff); // triggers enhanced DMA
}

uint8_t lpeek(uint32_t address)
{
  // Read the byte at <address> in 28-bit address space
  // XXX - Optimise out repeated setup etc
  // (separate DMA lists for peek, poke and copy should
  // save space, since most fields can stay initialised).

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=(address>>20);
  dmalist.option_81=0x81;
  dmalist.dest_mb=0x00; // dma_byte lives in 1st MB
  dmalist.end_of_options=0x00;

  dmalist.command=0x00; // copy
  dmalist.count=1;
  dmalist.source_addr=address&0xffff;
  dmalist.source_bank=(address>>16)&0x0f;
  dmalist.dest_addr=(uint16_t)&dma_byte;
  dmalist.dest_bank=0;

  do_dma();

  return dma_byte;
}

void lpoke(uint32_t address, uint8_t value)
{
  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=0x00; // dma_byte lives in 1st MB
  dmalist.option_81=0x81;
  dmalist.dest_mb=(address>>20);
  dmalist.end_of_options=0x00;

  dma_byte=value;
  dmalist.command=0x00; // copy
  dmalist.count=1;
  dmalist.source_addr=(uint16_t)&dma_byte;
  dmalist.source_bank=0;
  dmalist.dest_addr=address&0xffff;
  dmalist.dest_bank=(address>>16)&0x0f;

  do_dma();
  return;
}

void lcopy_safe(uint32_t src, uint32_t dst, uint16_t count)
{
  if (count>256) return;

  lcopy(src,(uint32_t)copy_buffer,count);
  lcopy((uint32_t)copy_buffer,dst,count);
}

void lcopy(uint32_t source_address, uint32_t destination_address, uint16_t count)
{
  if (!count) return;

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=source_address>>20;
  dmalist.option_81=0x81;
  dmalist.dest_mb=(destination_address>>20);
  dmalist.end_of_options=0x00;

  dmalist.command=0x00; // copy
  dmalist.count=count;
  dmalist.sub_cmd=0;
  dmalist.source_addr=source_address&0xffff;
  dmalist.source_bank=(source_address>>16)&0x0f;
  if (source_address>=0xd000 && source_address<0xe000)
    dmalist.source_bank|=0x80;
  dmalist.dest_addr=destination_address&0xffff;
  dmalist.dest_bank=(destination_address>>16)&0x0f;
  if (destination_address>=0xd000 && destination_address<0xe000)
    dmalist.dest_bank|=0x80;

  do_dma();
  return;
}

void lfill(uint32_t destination_address, uint8_t value, uint16_t count)
{
  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=0x00;
  dmalist.option_81=0x81;
  dmalist.dest_mb=destination_address>>20;
  dmalist.end_of_options=0x00;

  dmalist.command=0x03; // fill
  dmalist.sub_cmd=0;
  dmalist.count=count;
  dmalist.source_addr=value;
  dmalist.dest_addr=destination_address&0xffff;
  dmalist.dest_bank=(destination_address>>16)&0x0f;
  if (destination_address>=0xd000 && destination_address<0xe000)
    dmalist.dest_bank|=0x80;

  do_dma();
  return;
}

void m65_io_enable(void)
{
  // Gate C65 IO enable
  POKE(0xd02fU,0x47);
  POKE(0xd02fU,0x53);
  // Force to full speed
  POKE(0,65);
}
