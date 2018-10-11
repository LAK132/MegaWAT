#include "f65.h"

void patchFont(uint8_t *font)
{
    uint16_t glyph_count = font[0x80] | (font[0x81] << 8);

    uint16_t tile_map_start = font[0x82] | (font[0x83] << 8);
    uint16_t point_size = tile_map_start - 0x100;
    uint8_t *point = font + 0x100;

    uint32_t tile_array_start = font[0x84] | (font[0x85] << 8) | (font[0x86] << 16);
    uint32_t tile_map_size = tile_map_start - tile_array_start;
    uint8_t *map = font + tile_map_start;
    uint8_t *tile, *cards;

    uint8_t *array = font + tile_array_start;
    uint32_t i, j, k,
        glyph_height, glyph_width, glyph_size,
        map_pos, array_pos;

    tile_array_start /= 0x40;

    for(i = 0; i < point_size; i += 5)
    {
        map_pos = point[i + 2] | (point[i + 3] << 8) | (point[i + 4] << 16);
        tile = &map[map_pos];
        point[i + 2] = ((uint32_t)tile) & 0xFF;
        point[i + 3] = (((uint32_t)tile) >> 8) & 0xFF;
        point[i + 4] = (((uint32_t)tile) >> 16) & 0xFF;
        cards = tile + 4;
        glyph_height = (tile[0] + tile[1]);
        glyph_width = tile[2];
        glyph_size = glyph_height * glyph_width;
        for (j = 0; j < glyph_size; ++j)
        {
            // uint16_t card = cards[0] | (cards[1] << 8);
            cards[0] += tile_array_start & 0xFF;
            cards[1] += tile_array_start >> 8;
            cards += 2;
        }
    }
}