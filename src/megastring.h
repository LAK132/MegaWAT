#include "megaint.h"
#include "memory.h"

#ifndef MEGASTRING_H
#define MEGASTRING_H

typedef struct
{
    longptr_t ptr;
    uint16_t size; // size in bytes, not codes
} string_t;

#define string_size(strptr) ((string_length(strptr) + 1) * sizeof(uint16_t))
#define string_loff(str, pos) ((str).ptr + (((longptr_t)(pos)) * sizeof(uint16_t)))
#define string_peek(str, pos) \
    (lpeek(string_loff(str, pos)) | (((uint16_t)lpeek(string_loff(str, pos) + 1)) << 8))
#define string_poke(str, pos, code) { \
    lpoke(string_loff(str, pos), (code) & 0xFF); \
    lpoke(string_loff(str, pos) + 1, (code) >> 8); \
}
#define string_write_string(str, pos, wrtptr) \
    lcopy((wrtptr)->ptr, string_loff(str, pos), string_size(wrtptr))
#define string_write_string_safe(str, pos, wrtptr) \
    lcopy_safe((wrtptr)->ptr, string_loff(str, pos), string_size(wrtptr))
#define string_copy_substring(str, substr, from, to) \
    lcopy(string_loff(substr, from), (str).ptr, ((to) - (from)) * sizeof(uint16_t))
#define string_copy_substring_safe(str, substr, from, to) \
    lcopy_safe(string_loff(substr, from), (str).ptr, ((to) - (from)) * sizeof(uint16_t))
#define string_fill(str, u8char) lfill((str).ptr, u8char, (str).size)
#define string_clear(str) string_fill(str, 0)
#define string_remove_range(strptr, from, to) string_remove(strptr, from, (to) - (from))

uint16_t string_length(const string_t *str);
void string_insert(const string_t *str, const uint16_t position, const uint16_t count);
void string_insert_code(const string_t *str, const uint16_t position, const uint16_t code);
void string_insert_codes(const string_t *str, const uint16_t position, const uint16_t code, uint16_t count);
void string_insert_string(const string_t *str, const uint16_t position, const string_t *ins);
void string_remove(const string_t *str, const uint16_t position, uint16_t count);

#endif // MEGASTRING_H