#include "editor.h"

uint8_t active_slide;
uint8_t present_mode = 0;
uint8_t text_colour = 1;
uint8_t cursor_col = 0;
// Which line of text we are on
uint8_t text_line = 0;
// Rows where each line of text is drawn
#define EDITOR_END_LINE 60
#define EDITOR_MAX_LINES (EDITOR_END_LINE + 1)
uint32_t text_line_start[EDITOR_MAX_LINES];
uint32_t text_line_first_rows[EDITOR_END_LINE];
uint8_t text_line_count = 1;
// Physical rows on the slide (as compared to the lines of text that
// produce them).
// uint8_t current_row = 0;
// uint8_t next_row = 0;
#define CURRENT_ROW (text_line_first_rows[text_line])
#define NEXT_ROW (text_line_first_rows[text_line + 1])
#define END_ROW (text_line_first_rows[EDITOR_END_LINE])
#define CURRENT_ROW_HEIGHT (NEXT_ROW - CURRENT_ROW)

// XXX For now, have a fixed 128x30 buffer for text
// (We allow 128 instead of 100, so that we can have a number of font and attribute/colour changes
// in there.)
#define EDITOR_LINE_LEN 128
uint16_t editor_scratch[EDITOR_LINE_LEN];
uint16_t editor_buffer[SLIDE_SIZE / sizeof(uint16_t)];
uint32_t editor_buffer_size = sizeof(editor_buffer) / sizeof(uint16_t);

#define EDITOR_END_SLIDE 64
#define EDITOR_MAX_SLIDES (EDITOR_END_SLIDE + 1)
ptr_t slide_start[EDITOR_MAX_SLIDES];
uint32_t slide_number = 0;
uint8_t slide_colour[EDITOR_MAX_LINES];

#define HIDE_SPRITE(sprite) POKE(0xD015U, PEEK(0xD015U) & ~(1<<sprite))
#define SHOW_SPRITE(sprite) POKE(0xD015U, PEEK(0xD015U) | (1<<sprite))
#define TOGGLE_SPRITE(sprite) POKE(0xD015U, PEEK(0xD015U) ^ (1<<sprite))

#define HIDE_CURSOR() HIDE_SPRITE(0)
#define SHOW_CURSOR() SHOW_SPRITE(0)
#define TOGGLE_CURSOR() TOGGLE_SPRITE(0)

int8_t maxlen = 80;
uint8_t key = 0;
uint8_t mod = 0;
int16_t xx;
uint8_t h;
uint32_t sram, cram;
uint16_t cursor_toggle = 0;

void editor_show_slide_number(void);
void editor_hide_slide_number(void);

void editor_insert_line(uint8_t before)
{
    // Insert a new blank line before line #before
}

void editor_get_line_info(void)
{
    y = 0;
    text_line_start[y++] = 0;
    for (l = 0; y < EDITOR_MAX_LINES && l < editor_buffer_size;)
        if (!editor_buffer[l++])
            text_line_start[y++] = l;

    for (; y < EDITOR_MAX_LINES; ++y)
        text_line_start[y] = text_line_start[y-1] + 1;

    for (y = 0; y < EDITOR_END_LINE; ++y)
        text_line_first_rows[y] = y;
}

void editor_save_slide(void);
void editor_next_slide(void);
void editor_load_slide(void);
void editor_previous_slide(void);

void editor_initialise(void)
{
    videoSetSlideMode();

    active_slide = 0;
    videoSetActiveGraphicsBuffer(active_slide);
    videoSetActiveRenderBuffer(active_slide);

    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();

    // Fill the rest of the buffer with zeros
    lfill((ptr_t)editor_buffer, 0x00, sizeof(editor_buffer));

    editor_get_line_info();

    text_line_count = 0;
    text_line = 0;
    cursor_col = 0;

    lfill(SLIDE_DATA, 0, SLIDE_DATA_SIZE);
    slide_start[0] = SLIDE_DATA;
    for (y = 1; y < EDITOR_MAX_SLIDES; ++y)
        slide_start[y] = slide_start[y-1] + (EDITOR_END_LINE * 2);

    // default slide colours to 0x6
    lfill((ptr_t)slide_colour, 0x6, sizeof(slide_colour));

    // default to no file_name
    lfill((ptr_t)file_name, 0, sizeof(file_name));

    // make sure slide 2 is preloaded correctly
    editor_save_slide();
    editor_next_slide();
    editor_load_slide();
    editor_save_slide();
    editor_previous_slide();
    editor_load_slide();

    editor_show_slide_number();
}

void editor_stash_line(void)
{
    // Stash the line encoded in scratch buffer into the specified line

    // Can't stash end line
    if (text_line < EDITOR_END_LINE)
    {
        // Setup default font and attributes, so that we can notice when
        // switching.
        font_id = 0;
        text_colour = 1;

        y = 0;
        for (x = 0; x < scratch_rbuffer.glyph_count; ++x)
        {
            // Make sure we don't overflow and leave room for EOL
            if (y >= EDITOR_LINE_LEN - 2) break;

            // x == 0 -> make sure we we always start with the correct font/colour
            // Encode any required colour/attribute/font changes
            if (x == 0 || scratch_rbuffer.glyphs[x].colour_and_attributes != text_colour)
            {
                // Colour change, so write 0xe0XX, where XX is the colour
                editor_scratch[y++] = 0xE000 | (scratch_rbuffer.glyphs[x].colour_and_attributes & 0xFF);
                text_colour = scratch_rbuffer.glyphs[x].colour_and_attributes;
                if (y >= EDITOR_LINE_LEN - 2) break;
            }
            if (x == 0 || scratch_rbuffer.glyphs[x].font_id != font_id)
            {
                editor_scratch[y++] = 0xE100 | (scratch_rbuffer.glyphs[x].font_id & 0xFF);
                font_id = scratch_rbuffer.glyphs[x].font_id;
                if (y >= EDITOR_LINE_LEN - 2) break;
            }

            // Write glyph
            editor_scratch[y++] = scratch_rbuffer.glyphs[x].code_point;
        }
        // Add EOL to scratch
        editor_scratch[y++] = 0; // y is now the length of scratch used

        // We use line_num + 1 lots, so just do the calculation once
        c = text_line + 1;

        // Find how long this line is currently in the buffer
        // +1 is safe here because text_line_start is oversized by 1
        z = text_line_start[c] - text_line_start[text_line];

        // make sure we have enough space to stash the current line
        if (z > y)
        {
            // Space available is bigger than space needed, shrink
            x = z - y; // amount to shrink by
            lcopy_safe(
                (ptr_t)&editor_buffer[text_line_start[c]],
                (ptr_t)&editor_buffer[text_line_start[c] - x],
                (text_line_start[EDITOR_END_LINE] - text_line_start[c]) * sizeof(uint16_t)
            );
            lfill((ptr_t)&editor_buffer[text_line_start[EDITOR_END_LINE]], 0x00,
                sizeof(editor_buffer) - (text_line_start[EDITOR_END_LINE] * sizeof(uint16_t))
            );

            for (; c < EDITOR_MAX_LINES; ++c)
                text_line_start[c] -= x;
        }
        else if (y > z)
        {
            // Space available is smaller than space needed, expand
            x = y - z; // amout to expand by
            lcopy_safe(
                (ptr_t)&editor_buffer[text_line_start[c]],
                (ptr_t)&editor_buffer[text_line_start[c] + x],
                sizeof(editor_buffer) - ((text_line_start[c] + x) * sizeof(uint16_t))
            );

            for (; c < EDITOR_MAX_LINES; ++c)
                text_line_start[c] += x;
        }

        // copy scratch into main buffer
        // x2 because it's 16 bit
        lcopy((ptr_t)editor_scratch, (ptr_t)&editor_buffer[text_line_start[text_line]], y + y);
    }
}

void editor_render_string(const uint8_t *str)
{
    active_rbuffer = &scratch_rbuffer;
    for (; *str != 0; ++str)
        renderGlyph(*str, text_colour, ATTRIB_ALPHA_BLEND, cursor_col++);

    if (text_line < EDITOR_END_LINE)
    {
        // Check if this code point grew the height of the line.
        // If so, push everything else down before pasting
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) - CURRENT_ROW_HEIGHT;
        if (s > 0)
        {
            // Yes, it grew.
            // Now shift everything below this line down (on the SCREEN buffer)
            l = NEXT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            lcopy_safe(sram, sram + (s * screen_width), screen_rbuffer.screen_size - (l + screen_width));
            lcopy_safe(cram, cram + (s * screen_width), screen_rbuffer.screen_size - (l + screen_width));

            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;
        }
    }
}

void editor_render_glyph(uint16_t code_point)
{
    active_rbuffer = &scratch_rbuffer;
    renderGlyph(code_point, text_colour, ATTRIB_ALPHA_BLEND, cursor_col);

    if (text_line < EDITOR_END_LINE)
    {
        // Check if this code point grew the height of the line.
        // If so, push everything else down before pasting
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) - CURRENT_ROW_HEIGHT;
        if (s > 0)
        {
            // Yes, it grew.
            // Now shift everything below this line down (on the SCREEN buffer)
            l = NEXT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            lcopy_safe(sram, sram + (s * screen_width), screen_rbuffer.screen_size - (l + screen_width));
            lcopy_safe(cram, cram + (s * screen_width), screen_rbuffer.screen_size - (l + screen_width));

            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;
        }
    }
}

void editor_clear_line(void)
{
    active_rbuffer = &scratch_rbuffer;
    cursor_col = scratch_rbuffer.glyph_count + 1;
    while(cursor_col)
        deleteGlyph(cursor_col--);
    deleteGlyph(cursor_col);

    if (text_line < EDITOR_END_LINE)
    {
        // Check if this code point grew the height of the line.
        // If so, push everything else down before pasting
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) - CURRENT_ROW_HEIGHT;
        // Always make sure the row is atleast 1 line high
        if (scratch_rbuffer.max_above + scratch_rbuffer.max_below == 0)
        {
            // This row is now completely blank, so clear it as such
            l = CURRENT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
            lcopy((ptr_t)clear_pattern, sram, sizeof(clear_pattern));

            l = screen_width - sizeof(clear_pattern);
            // Fill out to whole line
            lcopy(sram, sram + sizeof(clear_pattern), l);

            // We don't want this line to actually be 0 tall, so increment
            ++s;
        }
        if (s < 0)
        {
            // Yes, it shrunk.
            // Now shift everything below this line up (on the SCREEN buffer)
            l = NEXT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            lcopy_safe(sram, sram + (s * (int32_t)screen_width), screen_rbuffer.screen_size - (l + screen_width));
            lcopy_safe(cram, cram + (s * (int32_t)screen_width), screen_rbuffer.screen_size - (l + screen_width));

            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;

            if (END_ROW < EDITOR_MAX_LINES)
            {
                // There is now space at the end of the screen, blank it out
                l = END_ROW * screen_width;
                sram = screen_rbuffer.screen_ram + l;
                cram = screen_rbuffer.colour_ram + l;
                // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
                lcopy((ptr_t)clear_pattern, sram, sizeof(clear_pattern));

                l = screen_width - sizeof(clear_pattern);
                // Fill out to whole line
                lcopy(sram, sram + sizeof(clear_pattern), l);

                l = (EDITOR_END_LINE - END_ROW) * screen_width;
                // Then copy it down over the next remaining rows.
                lcopy(sram, sram + screen_width, l - screen_width);
                // Fill the rest of the colour ram with 0x00
                lfill(cram, 0x00, l);
            }
        }
    }
}

void editor_delete_glyph(void)
{
    active_rbuffer = &scratch_rbuffer;
    deleteGlyph(cursor_col);

    if (text_line < EDITOR_END_LINE)
    {
        // Check if this code point grew the height of the line.
        // If so, push everything else down before pasting
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) - CURRENT_ROW_HEIGHT;
        // Always make sure the row is atleast 1 line high
        if (scratch_rbuffer.max_above + scratch_rbuffer.max_below == 0)
        {
            // This row is now completely blank, so clear it as such
            l = CURRENT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
            lcopy((ptr_t)clear_pattern, sram, sizeof(clear_pattern));

            l = screen_width - sizeof(clear_pattern);
            // Fill out to whole line
            lcopy(sram, sram + sizeof(clear_pattern), l);

            // We don't want this line to actually be 0 tall, so increment
            ++s;
        }
        if (s < 0)
        {
            // Yes, it shrunk.
            // Now shift everything below this line up (on the SCREEN buffer)
            l = NEXT_ROW * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            lcopy_safe(sram, sram + (s * (int32_t)screen_width), screen_rbuffer.screen_size - (l + screen_width));
            lcopy_safe(cram, cram + (s * (int32_t)screen_width), screen_rbuffer.screen_size - (l + screen_width));

            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;

            if (END_ROW < EDITOR_MAX_LINES)
            {
                // There is now space at the end of the screen, blank it out
                l = END_ROW * screen_width;
                sram = screen_rbuffer.screen_ram + l;
                cram = screen_rbuffer.colour_ram + l;
                // Fill screen RAM with 0x20 0x00 pattern, so that it is blank.
                lcopy((ptr_t)clear_pattern, sram, sizeof(clear_pattern));

                l = screen_width - sizeof(clear_pattern);
                // Fill out to whole line
                lcopy(sram, sram + sizeof(clear_pattern), l);

                l = (EDITOR_END_LINE - END_ROW) * screen_width;
                // Then copy it down over the next remaining rows.
                lcopy(sram, sram + screen_width, l - screen_width);
                // Fill the rest of the colour ram with 0x00
                lfill(cram, 0x00, l);
            }
        }
    }
}

void editor_fetch_line(void)
{
    // Fetch the specified line, and pre-render it into scratch

    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();

    // Get the start of the next line
    k = text_line_start[text_line + 1];
    // Check if it's past the end of the buffer
    if (k > editor_buffer_size) k = editor_buffer_size;

    c = cursor_col;
    cursor_col = 0xFF;
    for (m = text_line_start[text_line]; editor_buffer[m] && m < k; ++m)
    {
        // Reserved code points are used to record colour and other attribute changes
        switch (editor_buffer[m] & 0xFF00)
        {
            case 0xE000: {
                // Set colour and attributes
                text_colour = editor_buffer[m] & 0xFF;
            } break;
            case 0xE100: {
                setFont(editor_buffer[m] & 0xFF);
            } break;
            default: {
                // if this is actually a code point, render the relevant glyph
                if ((editor_buffer[m] < 0xD800) || (editor_buffer[m] >= 0xF800))
                    editor_render_glyph(editor_buffer[m]);
            } break;
        }
    }
    screen_rbuffer.rows_used = CURRENT_ROW;
    outputLineToRenderBuffer();
    cursor_col = c;
}

void editor_show_cursor(void)
{
    // Put cursor in initial position
    xx = 5;
    y = 30;
    POKE(0xD000, xx & 0xFF);
    POKE(0xD001, y);
    if (xx & 0x100)
        POKE(0xD010U, 0x01);
    else
        POKE(0xD010U, 0);
    if (xx & 0x200)
        POKE(0xD05FU, PEEK(0xD05FU)|0x01);
    else
        POKE(0xD05FU, PEEK(0xD05FU)&0xfe);
}

void editor_update_cursor(void)
{
    if (cursor_col > scratch_rbuffer.glyph_count)
        cursor_col = scratch_rbuffer.glyph_count;

    // Work out where cursor should be
    xx = 15; // Fudge factor
    for (x = 0; x < cursor_col; ++x)
        xx += (scratch_rbuffer.glyphs[x].columns * 8) - scratch_rbuffer.glyphs[x].trim_pixels;
    // Work out cursor height
    // Note: Sprites use V200 dimensions, thus 4 V200 pixels per 8 V400 pixels.
    h = 4 * (scratch_rbuffer.max_above + scratch_rbuffer.max_below);
    if (h < 4)
        h = 4;
    y = 28 + text_line_first_rows[text_line] * 4;
    // Set extended Y height to match required height.
    POKE(0xD056, h);
    // Make cursor be text colour (will alternate to another colour as well)
    POKE(0xD027U, text_colour & 0xF);
    // Move sprite to there
    POKE(0xD000, xx & 0xFF);
    POKE(0xD001, y);
    if (xx & 0x100)
        POKE(0xD010U, 0x01);
    else
        POKE(0xD010U, 0);
    if (xx & 0x200)
        POKE(0xD05FU, PEEK(0xD05FU)|0x01);
    else
        POKE(0xD05FU, PEEK(0xD05FU)&0xfe);
}

void editor_insert_codepoint(uint16_t code_point)
{
    // Shift to fix ASCII vs PETSCII for the C64 font
    if (!font_id && code_point >= 0x60)
        code_point -= 0x60;

    z = scratch_rbuffer.glyph_count;
    editor_render_glyph(code_point);
    screen_rbuffer.rows_used = CURRENT_ROW;
    outputLineToRenderBuffer();

    // Only advance cursor if the glyph was actually rendered
    if (scratch_rbuffer.glyph_count > z)
        ++cursor_col;
    if (cursor_col > scratch_rbuffer.glyph_count)
        cursor_col = scratch_rbuffer.glyph_count;
}

void editor_save_slide(void)
{
    l = (text_line_start[EDITOR_END_LINE] - text_line_start[0]) * sizeof(uint16_t);

    // We use slide_number + 1 lots, so just do the calculation once
    c = slide_number + 1;

    // Find how long this line is currently in the buffer
    // +1 is safe here because text_line_first_rows is oversized by 1
    j = slide_start[c] - slide_start[slide_number];

    // make sure we have enough space to stash the current line
    if (j > l)
    {
        // Space available is bigger than space needed, shrink
        i = j - l; // amount to shrink by
        // make sure there is at least 1 character (null) in the line
        if (!l) --i;

        lcopy_safe(
            slide_start[c],
            slide_start[c] - i,
            (uint16_t)(slide_start[EDITOR_END_SLIDE] - slide_start[c])
        );

        for (; c < EDITOR_MAX_SLIDES; ++c)
            slide_start[c] -= i;
    }
    else if (l > j)
    {
        // Space available is smaller than space needed, expand
        i = l - j; // amout to expand by

        lcopy_safe(
            slide_start[c],
            slide_start[c] + i,
            (uint16_t)((SLIDE_DATA + SLIDE_DATA_SIZE) - (longptr_t)(slide_start[c] + i))
        );

        for (; c < EDITOR_MAX_SLIDES; ++c)
            slide_start[c] += i;
    }
    // copy scratch into main buffer
    // x2 because it's 16 bit
    lcopy((ptr_t)editor_buffer, slide_start[slide_number], l);
}

void editor_load_slide(void)
{
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();
    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();

    lfill((ptr_t)editor_buffer, 0x00, sizeof(editor_buffer));

    // Copy the next slides buffer into editor buffer
    j = slide_start[slide_number + 1] - slide_start[slide_number];
    if (j > sizeof(editor_buffer))
        j = sizeof(editor_buffer);

    lcopy(slide_start[slide_number], (ptr_t)editor_buffer, j);

    // Update line starts for new buffer
    editor_get_line_info();
    w = text_line;
    for (text_line = 0; text_line < EDITOR_MAX_LINES; ++text_line)
    {
        editor_fetch_line();
        editor_stash_line();
    }
    text_line = w;
    editor_update_cursor();
}

void editor_next_slide(void)
{
    if (slide_number + 1 < EDITOR_END_SLIDE)
    {
        // Switch active slide (next slide)
        if (active_slide) // SLIDE1 active
            active_slide = 0;
        else // SLIDE0 active
            active_slide = 1;

        // Change the screen address
        videoSetActiveGraphicsBuffer(active_slide);
        // Don't change the render buffer address yet
        // so we can pre-render the next slide without
        // affecting the current slide

        // Move current slide (SLIDE0/1) to previous slide (SLIDE2)
        lcopy(active_slide ? SLIDE0_SCREEN_RAM : SLIDE1_SCREEN_RAM, SLIDE2_SCREEN_RAM, SLIDE_SIZE);
        lcopy(active_slide ? SLIDE0_COLOUR_RAM : SLIDE1_COLOUR_RAM, SLIDE2_COLOUR_RAM, SLIDE_SIZE);

        ++slide_number;
        // Pre-render the next slide
        if (slide_number + 1 < EDITOR_END_SLIDE)
        {
            ++slide_number;
            editor_load_slide();
            --slide_number;
        }

        // Change the render buffer address
        videoSetActiveRenderBuffer(active_slide);

        // Set the border colour
        POKE(0xD020, slide_colour[slide_number]);
        // Set the background colour
        POKE(0xD021, slide_colour[slide_number]);
    }
}

void editor_previous_slide(void)
{
    if (slide_number)
    {
        // Move previous slide (SLIDE2) to next slide (SLIDE1/0)
        lcopy(SLIDE2_SCREEN_RAM, active_slide ? SLIDE0_SCREEN_RAM : SLIDE1_SCREEN_RAM, SLIDE_SIZE);
        lcopy(SLIDE2_COLOUR_RAM, active_slide ? SLIDE0_COLOUR_RAM : SLIDE1_COLOUR_RAM, SLIDE_SIZE);

        // Switch active slide (now previous slide)
        if (active_slide) // SLIDE1 active
            active_slide = 0;
        else // SLIDE0 active
            active_slide = 1;

        // Change the screen address
        videoSetActiveGraphicsBuffer(active_slide);
        // Don't change the render buffer address yet
        // so we can pre-render the previous slide without
        // affecting the current slide

        --slide_number;
        // Pre-render the previous slide
        if (slide_number)
        {
            videoSetActiveRenderBuffer(2);
            --slide_number;
            editor_load_slide();
            ++slide_number;
        }

        // Change the render buffer address
        videoSetActiveRenderBuffer(active_slide);

        // Set the border colour
        POKE(0xD020, slide_colour[slide_number]);
        // Set the background colour
        POKE(0xD021, slide_colour[slide_number]);
    }
}

void editor_goto_slide(uint8_t num)
{
    if (slide_number == num)
        return;

    if (slide_number > num)
        while (slide_number > num)
            editor_previous_slide();
    else
        while(slide_number < num)
            editor_next_slide();
}

FILE *file;
uint32_t kk, cc;
void editor_process_special_key(uint8_t key)
{
    kk = 0; // if the cursor was moved
    if (present_mode) switch (key)
    {
        case 0x0D:
        case 0x20:
        case 0x11:
        case 0x1D: { // next slide
            editor_next_slide();
        } break;
        case 0x91:
        case 0x9D: { // previous slide
            editor_previous_slide();
        } break;
        case 0x03:
        case 0xF5: {
            // XXX - Reload editor buffer for current slide
            // XXX - Unhide cursor
            present_mode = 0;
            editor_load_slide();
            text_line = 0;
            editor_fetch_line();
            editor_show_slide_number();
        } break;
        default: break;
    }
    else switch (key)
    {
        // Commodore colour selection keys
        // CTRL
        case 0x90: text_colour = (text_colour & 0xF0) | COLOUR_BLACK; break;
        case 0x05: text_colour = (text_colour & 0xF0) | COLOUR_WHITE; break;
        case 0x1C: text_colour = (text_colour & 0xF0) | COLOUR_RED; break;
        case 0x9F: text_colour = (text_colour & 0xF0) | COLOUR_CYAN; break;
        case 0x9C: text_colour = (text_colour & 0xF0) | COLOUR_PURPLE; break;
        case 0x1E: text_colour = (text_colour & 0xF0) | COLOUR_GREEN; break;
        case 0x1F: text_colour = (text_colour & 0xF0) | COLOUR_BLUE; break;
        case 0x9E: text_colour = (text_colour & 0xF0) | COLOUR_YELLOW; break;
        // SUPER
        case 0x81: text_colour = (text_colour & 0xF0) | COLOUR_ORANGE; break;
        case 0x95: text_colour = (text_colour & 0xF0) | COLOUR_BROWN; break;
        case 0x96: text_colour = (text_colour & 0xF0) | COLOUR_PINK; break;
        case 0x97: text_colour = (text_colour & 0xF0) | COLOUR_DARKGREY; break;
        case 0x98: text_colour = (text_colour & 0xF0) | COLOUR_MEDIUMGREY; break;
        case 0x99: text_colour = (text_colour & 0xF0) | COLOUR_LIGHTGREEN; break;
        case 0x9A: text_colour = (text_colour & 0xF0) | COLOUR_LIGHTBLUE; break;
        // The following need fixes to the ASCII keyboard scanner
        case 0x9B: text_colour = (text_colour & 0xF0) | COLOUR_LIGHTGREY; break;
        // case 0x92: text_colour |= ATTRIB_BLINK; break;

        // CONTROL-u for underline
        case 0x15: text_colour ^= ATTRIB_UNDERLINE; break;
        // CONTROL-r (or RVS ON key once ASCII keyboard scanner fixed) for reverse
        case 0x12: text_colour ^= ATTRIB_REVERSE; break;
        // CONTROL-b for blink
        case 0x02: text_colour ^= ATTRIB_BLINK; break;

        // Background colour
        case 0xF1: {
            slide_colour[slide_number] = 0xF & (slide_colour[slide_number] + 1);
            // Set the border colour
            POKE(0xD020, slide_colour[slide_number]);
            // Set the background colour
            POKE(0xD021, slide_colour[slide_number]);
        } break;
        case 0xF2: {
            if (0xF & slide_colour[slide_number])
                slide_colour[slide_number] = 0xF & (slide_colour[slide_number] - 1);
            else
                slide_colour[slide_number] = 0x0F;
            // Set the border colour
            POKE(0xD020, slide_colour[slide_number]);
            // Set the background colour
            POKE(0xD021, slide_colour[slide_number]);
        } break;

        // Backspace (with CONTROL for DEL?)
        case 0x14: {
            if (cursor_col)
            {
                active_rbuffer = &scratch_rbuffer;
                --cursor_col;
                editor_delete_glyph();
                editor_update_cursor();
                screen_rbuffer.rows_used = CURRENT_ROW;
                outputLineToRenderBuffer();
                kk = 1;
            }
            // else if (current_row && text_line)
            else if (text_line)
            {
                // Backspace from start of line
                // XXX - Should combine remainder of line with previous line (if it will fit)
                // XXX - Shuffle up the rest of the screen,
                // XXX - Select the previous line
                // XXX - Re-render the previous line
                //       (And shuffle everything down in the process if this line grew taller)
                editor_stash_line();
                --text_line;
                editor_fetch_line();
                cursor_col = scratch_rbuffer.glyph_count;
                editor_update_cursor();
                kk = 1;
            }
        } break;

        // Cursor navigation within a line
        case 0x9d: if (cursor_col) --cursor_col; kk = 1; break;
        case 0x1d: if (cursor_col < scratch_rbuffer.glyph_count) ++cursor_col; kk = 1; break;

        // Cursor navigation between lines
        // Here we adjust which line we are editing,
        // Fix cursor position if it would be beyond the end of the line
        case 0x0d: { // RETURN
            // Break line into two
            editor_insert_line(text_line + 1);
            cursor_col = 0;
        } // don't break
        case 0x11: { // Cursor down
            if (text_line < EDITOR_MAX_LINES)
            {
                editor_stash_line();
                ++text_line;
                editor_fetch_line();
                editor_update_cursor();
                kk = 1;
            }
        } break;
        case 0x91: { // Cursor up
            if (text_line)
            {
                editor_stash_line();
                --text_line;
                editor_fetch_line();
                editor_update_cursor();
                kk = 1;
            }
        } break;
        case 0xED:
        case 0xEE: { // change slide
            editor_stash_line();
            editor_save_slide();
            if (key == 0xC2 || ((key == 0xED || key == 0xEE) && (mod & MOD_SHIFT)))
                editor_previous_slide();
            else
                editor_next_slide();
            editor_load_slide();
            editor_fetch_line();
            editor_show_slide_number();
        } break;
        case 0xF5: {
            editor_stash_line();
            editor_save_slide();
            present_mode = 1;
            editor_hide_slide_number();
        } break;
        case 0x09: { // TAB
            editor_insert_codepoint(0x20);
            editor_insert_codepoint(0x20);
            editor_insert_codepoint(0x20);
            editor_insert_codepoint(0x20);
        } break;
        case 0xCF: { // MEGA O
            // Open
            editor_stash_line();
            editor_save_slide();

            lcopy((ptr_t)"default.mwt", (ptr_t)file_name, sizeof("default.mwt"));
            if (fileio_load_pres())
            {
                console_write_au32(errno);
                console_write_astr(strerror(errno));
            }
            else
            {
                editor_initialise();
            }

            // XXX - switch to blank slide
            // XXX - use slide to show SD card contents
            // XXX - use hardware reverse to show selection
            // XXX - on RETURN: load presentation
            // XXX - on ESC: return to editing previous presentation
            editor_load_slide();
            editor_fetch_line();
        } break;
        case 0xD3: { // MEGA S
            // if previously saved and SHIFT not held, save silently
            if (file_name[0] != 0 && mod != MOD_SHIFT)
            {
                // Save
                editor_stash_line();
                editor_save_slide();
                lcopy((ptr_t)"default.mwt", (ptr_t)file_name, sizeof("default.mwt"));
                if (fileio_save_pres())
                {
                    console_write_au32(errno);
                    console_write_astr(strerror(errno));
                }
                editor_load_slide();
                editor_fetch_line();
            }
            else
            {
                // Save As
                editor_stash_line();
                editor_save_slide();
                // XXX - switch to blank slide
                // XXX - use scratch buffer for input
                // XXX - on RETURN: save
                // XXX - on ESC: cancel
                lcopy((ptr_t)"default.mwt", (ptr_t)file_name, sizeof("default.mwt"));
                if (fileio_create_pres())
                {
                    console_write_au32(errno);
                    console_write_astr(strerror(errno));
                }
                if (fileio_save_pres())
                {
                    console_write_au32(errno);
                    console_write_astr(strerror(errno));
                }

                editor_load_slide();
                editor_fetch_line();
            }
        } break;
        case 0xCE: { // MEGA N
            // New
            lcopy((ptr_t)"default.mwt", (ptr_t)file_name, sizeof("default.mwt"));
            if (fileio_create_pres())
            {
                HIDE_CURSOR();
                editor_stash_line();
                editor_save_slide();
                kk = text_line;
                cc = cursor_col;
                // XXX - "Are you sure?" prompt

                active_rbuffer = &screen_rbuffer;
                clearRenderBuffer();
                active_rbuffer = &scratch_rbuffer;
                clearRenderBuffer();

                text_line = 0;
                editor_fetch_line();
                editor_clear_line();
                editor_render_string("start a new presentation? unsaved changes will be lost");
                screen_rbuffer.rows_used = CURRENT_ROW;
                outputLineToRenderBuffer();

                text_line = 2;
                editor_fetch_line();
                editor_clear_line();
                editor_render_string("yes: RETURN");
                screen_rbuffer.rows_used = CURRENT_ROW;
                outputLineToRenderBuffer();

                text_line = 4;
                editor_fetch_line();
                editor_clear_line();
                editor_render_string("no:  ESC");
                screen_rbuffer.rows_used = CURRENT_ROW;
                outputLineToRenderBuffer();

                while (READ_KEY() != KEY_ESC && READ_KEY() != KEY_RETURN) continue;
                if (READ_KEY() == KEY_RETURN)
                {
                    // reinitalise
                    editor_goto_slide(0);
                    editor_initialise();
                }
                else
                {
                    // return to normal editing
                    text_line = kk;
                    cursor_col = cc;
                }
                editor_load_slide();
                editor_fetch_line();
                kk = 0;
            }
        } break;
        default: break;
    }
    if (kk && cursor_col > 0 && cursor_col <= scratch_rbuffer.glyph_count)
    {
        if (font_id != scratch_rbuffer.glyphs[cursor_col-1].font_id)
            setFont(scratch_rbuffer.glyphs[cursor_col-1].font_id);
        if (text_colour != scratch_rbuffer.glyphs[cursor_col-1].colour_and_attributes)
            text_colour = scratch_rbuffer.glyphs[cursor_col-1].colour_and_attributes;
    }
}

uint8_t remainder,yy;
uint8_t slide_num_message[8]={19+64,12,9,4,5,' ','0','0'}; // "Slide 00"
uint8_t edit_mode_message[8]={5+64,4,9,20,9,14,7,'.'};
uint8_t present_mode_message[8]={16+64,18,5,19,5,14,20,'.'};


void editor_show_slide_number(void)
{

  // Clear sprite (8x21 bytes) and write
  // info in it
  lfill(0x0500,0x00,0x100);

  // Produce message to show on the sprite
  slide_num_message[6]='0';
  remainder=slide_number+1;
  while(remainder>9) { remainder-=10; slide_num_message[6]++; }
  slide_num_message[7]=0x30+remainder;
  if (slide_num_message[6]=='0') {
    slide_num_message[6]=slide_num_message[7];
    slide_num_message[7]=' ';
  }
  for(remainder=0;remainder<8;remainder++)
    for(yy=0;yy<8;yy++)
      POKE(0x0500+remainder+yy*8,lpeek(0x2D800+(slide_num_message[remainder]*8)+yy));

  if (present_mode) {
    for(remainder=0;remainder<8;remainder++)
      for(yy=0;yy<8;yy++)
    POKE(0x0540+remainder+yy*8,lpeek(0x2D800+(present_mode_message[remainder]*8)+yy));
  } else {
    // Display editing in reverse text, so that it is more obvious
    for(remainder=0;remainder<8;remainder++)
      for(yy=0;yy<8;yy++)
    POKE(0x0540+remainder+yy*8,lpeek(0x2DC00+(edit_mode_message[remainder]*8)+yy));
  }


  // Set sprite data fetch area to $0500
  POKE(2041,0x0500/0x40);
  // Extended width (64 pixels wide)
  POKE(0xD057U,0x02);

  // Position sprite 2 near lower right corner
  POKE(0xD003U,0xF7);
  POKE(0xD002U,0xa0);
  POKE(0xD010U,0x00);
  POKE(0xD05FU,PEEK(0xD05FU)|0x02);

  // Make sprite expanded in X any Y
  // POKE(0xD017U,PEEK(0xD017U)|2);
  POKE(0xD01DU,PEEK(0xD01DU)|2);

  // Set sprite colour to light green
  POKE(0xD028,0x0d);

  // Make sprite 2 visible again
  POKE(0xD015U,PEEK(0xD015U)|0x02);

  // Start with message fully faded in
  POKE(0xD074U,2);  // Make sprite alpha blended
  POKE(0xD075U,0xFF); // Alpha blend set to fully visible

}

void editor_hide_slide_number(void)
{
    lfill(0x0500,0x00,0x100);
}

void editor_poll_keyboard(void)
{
    #ifndef __MEGA65__
    while (key != KEY_ESC)
    #else
    for (;;)
    #endif
    {
        // TOGGLE_BACK();
        key = READ_KEY();
        if (key)
        {
            // Allow enough time for modifier flags to get asserted
            mod = READ_MOD();
            for (m = 0; m < 3000; ++m)
                mod |= READ_MOD();

            while (READ_KEY())
            {
                // read modifier before clearing the key
                mod |= READ_MOD();
                // clear the key
                READ_KEY() = 1;
            }

            // Control+SHIFT <0-9> = select font
            if (!present_mode && (key >= 0x21 && key <= 0x29) && (mod & MOD_CTRL))
                setFont(key - 0x21);
            else if (!present_mode && key >= ' ' && key <= 0x7e)
                editor_insert_codepoint(key);
            else
                editor_process_special_key(key);

            editor_update_cursor();

            // Make sure cursor is on when typing
            SHOW_CURSOR();

            // If key is still set it might be triggering too fast, so manually wait some time
            if (READ_KEY() == key)
                for (l = 0; l < 35000; l++) READ_KEY() = 1;
        }
        else
        {
            if (present_mode)
            {
                HIDE_CURSOR();
            }
            else if (PEEK(0xD012U) > 0xF8)
            {
                if (!(cursor_toggle += 0x10))
                    TOGGLE_CURSOR(); // Toggle cursor on/off quickly
            }
            if (PEEK(0xD012U) == 0xE0)
            {
                // Fade out slide indicator
                if (PEEK(0xD075U))
                    POKE(0xD075U,PEEK(0xD075U)-1);
                else
                    // Disable sprite after it has faded out
                    HIDE_SPRITE(1);
                // Take a little time so it doesn't get retriggered so fast
                for(key = 0; key < 16; ++key) continue;
            }
        }
    }
}
