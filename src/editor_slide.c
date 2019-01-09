#include "editor.h"

line_t editor_slide_get_line(slide_t *slide, uint8_t number)
{
    static uint32_t i;
    for (i = 0; number != 0; ++i)
        if (slide->lines[i] == 0)
            --number;
    return &(slide->lines[i]);
}

// shift characters after <position> to the right by <count> to allow characters to be inserted before <position>
void editor_slide_insert_line_space(slide_t *slide, uint8_t number, uint8_t position, uint8_t count)
{
    static line_t start, end;
    static uint8_t i = 0;
    if (count == 0 || position >= (EDITOR_LINE_LEN - 1)) // -1 because null terminated
        return;
    start = &(editor_slide_get_line(slide, number)[position]);
    end = editor_slide_get_line(slide, EDITOR_END_LINE);
    if (((uint32_t)end - (uint32_t)start) + (count * sizeof(*start)) > sizeof(slide->lines))
        return;
    lcopy_safe(start, &(start[count]), (uint32_t)end - (uint32_t)start);
    for (i = 0; i < count; ++i)
        start[i] = 0x20; // fill with spaces
}

// shift characters from <position + count> to the left by <count> to remove characters between <position> and <position + count>
void editor_slide_remove_line_space(slide_t *slide, uint8_t number, uint8_t position, uint8_t count)
{
    static line_t start, end;
    static uint8_t i = 0;
    if (count == 0)
        return;
    start = &(editor_slide_get_line(slide, number)[position]);
    end = editor_slide_get_line(slide, EDITOR_END_LINE);
    lcopy_safe(&(start[count]), start, (uint32_t)end - (uint32_t)start);
    lfill(end - count, 0, (count * sizeof(*end)));
    // lfill(((uint32_t)end) - (count * sizeof(*end)), 0, (count * sizeof(*end)));
}

// copy a slide from one memory space to another
void editor_copy_slide(slide_t *restrict src, slide_t *restrict dst, editor_copy_mode_t mode)
{
    dst->colour = src->colour;
    dst->resolution = src->resolution;
    lcopy(src->fontPack, dst->fontPack, sizeof(src->fontPack));
    lcopy(src->lines, dst->lines, sizeof(src->lines));
}

void editor_slide_insert_lines(slide_t *slide, uint8_t number, uint8_t count)
{
    static line_t line;
    line = editor_slide_get_line(slide, number);
    editor_slide_insert_line_space(slide, number, 0, count);
    for (; count > 0; --count)
        line[count] = 0;
}

void editor_slide_remove_lines(slide_t *slide, uint8_t number, uint8_t count)
{
    static uint32_t start, end;
    start = editor_slide_get_line(slide, number);
    end = editor_slide_get_line(slide, number + count);
    // abuse the fact that remove line space doesn't check for overrun
    editor_slide_remove_line_space(slide, number, 0, end - start);
}