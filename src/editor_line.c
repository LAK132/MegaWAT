#include "editor.h"

uint8_t editor_line_length(line_t *line)
{
    static uint8_t rtn;
    for (rtn = 0; (*line)[rtn] != 0 && rtn < (EDITOR_LINE_LEN - 1); ++rtn);
    return rtn;
}

void editor_line_trim(line_t *line, uint8_t length)
{
    lfill(&((*line)[length]), 0, (EDITOR_LINE_LEN - length) * sizeof(**line));
}

line_t editor_next_line(line_t line)
{
    while (*(line++) != 0);
    return line;
}

// copy a line from one memory space to another
void editor_copy_line(line_t *restrict src, line_t *restrict dst)
{
    static line_t end;
    end = editor_next_line(src) + 1;
    lcopy(src, dst, (uint32_t)end - (uint32_t)src);
}

// // shift characters after <position> to the right by <count> to allow characters to be inserted before <position>
// void editor_line_insert_space(line_t *line, uint8_t position, uint8_t count)
// {
//     if (count == 0 || position >= (EDITOR_LINE_LEN - 1)) // -1 because null terminated
//         return;
//     lcopy_safe(&((*line)[position]), &((*line)[position + count]),
//         ((EDITOR_LINE_LEN - 1) - (position + count)) * sizeof(**line));
//     (*line)[EDITOR_LINE_LEN-1] = 0; // be doubly sure it's still null terminaed
// }

// // shift characters from <position + count> to the left by <count> to remove characters between <position> and <position + count>
// void editor_line_remove_space(line_t *line, uint8_t position, uint8_t count)
// {
//     if (count == 0)
//         return;
//     lcopy_safe(&((*line)[position + count]), &((*line)[position]),
//         (EDITOR_LINE_LEN - (position + count)) * sizeof(**line));
//     // fill end with 0
//     // abuse the fact that we moved the null terminator
//     editor_line_trim(line, editor_line_length(line));
// }