#include "megastring.h"

uint16_t string_length(const string_t *str)
{
    static uint16_t i;
    for (i = 0; i < str->size; ++i)
        if ((lpeek(str->ptr + i + i) == 0) &&
            (lpeek(str->ptr + i + i + 1) == 0))
            break;
    return i;
}

void string_insert(const string_t *str, const uint16_t position, const uint16_t count)
{
    static longptr_t from, to;
    if (count == 0 || position >= str->size)
        return;
    from = str->ptr + (position * sizeof(uint16_t));
    to = from + (count * sizeof(uint16_t));
    lcopy_safe(from, to, (str->ptr + str->size) - to);
}

void string_insert_code(const string_t *str, const uint16_t position, const uint16_t code)
{
    string_insert(str, position, 1);
    string_poke(*str, position, code);
}

void string_insert_codes(const string_t *str, const uint16_t position, const uint16_t code, uint16_t count)
{
    while (count --> 0)
        string_insert_code(str, position, code);
}

void string_insert_string(const string_t *str, const uint16_t position, const string_t *ins)
{
    static uint16_t len;
    len = string_length(ins);
    string_insert(str, position, len); // make room
    lcopy(ins->ptr, string_loff(*str, position), len * sizeof(uint16_t));
}

void string_remove(const string_t *str, const uint16_t position, uint16_t count)
{
    static longptr_t from, to;
    if (count == 0 || position >= str->size)
        return;
    count *= sizeof(uint16_t);
    to = str->ptr + (position * sizeof(uint16_t));
    from = to + count;
    lcopy_safe(from, to, (str->ptr + str->size) - from);
    while (count --> 0)
        string_poke(*str, str->size - count, 0);
}