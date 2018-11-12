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
#define CURRENT_ROW_HEIGHT (NEXT_ROW - CURRENT_ROW)

// XXX For now, have a fixed 128x30 buffer for text
// (We allow 128 instead of 100, so that we can have a number of font and attribute/colour changes
// in there.)
#define EDITOR_LINE_LEN 128
uint16_t editor_scratch[EDITOR_LINE_LEN];
uint16_t editor_buffer[SLIDE_SIZE / sizeof(uint16_t)];
uint32_t editor_buffer_size = sizeof(editor_buffer) / sizeof(uint16_t);

#define EDITOR_END_SLIDE 10
#define EDITOR_MAX_SLIDES (EDITOR_END_SLIDE + 1)
ptr_t slide_start[EDITOR_MAX_SLIDES];
uint32_t slide_number = 0;

int8_t maxlen = 80;
uint8_t key = 0;
uint8_t mod = 0;
int16_t xx;
uint8_t h;
uint32_t sram, cram;
uint16_t cursor_toggle = 0;

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
void editor_load_slide(void);

void editor_initialise(void)
{
    videoSetSlideMode();

    active_slide = 0;
    videoSetActiveGraphicsBuffer(active_slide);
    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();

    videoSetActiveRenderBuffer(active_slide);
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();

    // Fill the rest of the buffer with zeros
    lfill((ptr_t)&editor_buffer[0], 0x00,
        sizeof(editor_buffer));

    editor_get_line_info();

    text_line_count = 0;

    lfill(SLIDE_DATA, 0, SLIDE_DATA_SIZE);
    slide_start[0] = SLIDE_DATA;
    for (y = 1; y < EDITOR_MAX_SLIDES; ++y)
        slide_start[y] = slide_start[y-1] + (EDITOR_END_LINE * 2);
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
        lcopy((ptr_t)&editor_scratch[0], (ptr_t)&editor_buffer[text_line_start[text_line]], y + y);
    }
}

void editor_render_glyph(uint16_t code_point)
{
    active_rbuffer = &scratch_rbuffer;
    renderGlyph(code_point, text_colour, ATTRIB_ALPHA_BLEND, cursor_col);

    // Check if this code point grew the height of the line.
    // If so, push everything else down before pasting
    if (text_line < EDITOR_END_LINE)
    {
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) - CURRENT_ROW_HEIGHT;
        if (s > 0)
        {
            // Yes, it grew.
            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;

            // Now shift everything down (on the SCREEN buffer)
            l = text_line_first_rows[text_line] * screen_width;
            sram = screen_rbuffer.screen_ram + l;
            cram = screen_rbuffer.colour_ram + l;
            lcopy_safe(sram, sram + screen_width, screen_rbuffer.screen_size - (l + screen_width));
            lcopy_safe(cram, cram + screen_width, screen_rbuffer.screen_size - (l + screen_width));
        }
    }
}

void editor_fetch_line(void) //uint8_t line_num)
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
        POKE(0xD05FU, 0x01);
    else
        POKE(0xD05FU, 0);
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
        POKE(0xD05FU, 0x01);
    else
        POKE(0xD05FU, 0);
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
    lcopy((ptr_t)&editor_buffer[0], slide_start[slide_number], l);
}

void editor_load_slide(void)
{
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();
    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();

    lfill((ptr_t)&editor_buffer[0], 0x00, sizeof(editor_buffer));

    // Copy the next slides buffer into editor buffer
    j = slide_start[slide_number + 1] - slide_start[slide_number];
    if (j > sizeof(editor_buffer))
        j = sizeof(editor_buffer);

    lcopy(slide_start[slide_number], (ptr_t)&editor_buffer[0], j);

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

void editor_process_special_key(uint8_t key)
{
    k = 0; // if the cursor was moved
    if (present_mode) switch (key)
    {
        case 0x0D:
        case 0x20:
        case 0x11:
        case 0x1D: { // next slide
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
            } //else TOGGLE_BACK();
        } break;
        case 0x91:
        case 0x9D: { // previous slide
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
            } //else TOGGLE_BACK();
        } break;
        case 0x03:
        case 0xF5: {
            // XXX - Reload editor buffer for current slide
            // XXX - Unhide cursor
            present_mode = 0;
            editor_load_slide();
            text_line = 0;
            editor_fetch_line();
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

        // Backspace (with CONTROL for DEL?)
        case 0x14: {
            if (cursor_col)
            {
                active_rbuffer = &scratch_rbuffer;
                --cursor_col;
                deleteGlyph(cursor_col);
                screen_rbuffer.rows_used = NEXT_ROW;

                // XXX - This is a hack to prevent in-line tearing
                // XXX - Figure out what is actually causing the issue
                editor_stash_line();
                editor_fetch_line();
                editor_update_cursor();
                screen_rbuffer.rows_used = CURRENT_ROW;
                outputLineToRenderBuffer();

                if (screen_rbuffer.rows_used < NEXT_ROW)
                {
                    // Deleting a character reduced the row height,
                    // So shuffle up the rows below, and fill in the
                    // bottom of the screen.

                    // Copy rows up
                    lcopy(screen_rbuffer.screen_ram + NEXT_ROW * screen_width,
                        screen_rbuffer.screen_ram + (screen_rbuffer.rows_used * screen_width),
                        screen_rbuffer.screen_size - (screen_width * (60 - NEXT_ROW))
                    );
                    lcopy(screen_rbuffer.colour_ram + NEXT_ROW * screen_width,
                        screen_rbuffer.colour_ram + (screen_rbuffer.rows_used * screen_width),
                        screen_rbuffer.screen_size - (screen_width * (60 - NEXT_ROW))
                    );

                    // XXX Fill in bottom of screen

                    // Adjust starting rows for following lines
                    x = NEXT_ROW - screen_rbuffer.rows_used;
                    for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                        text_line_first_rows[y] -= x;
                }
                // next_row = screen_rbuffer.rows_used;
                k = 1;
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
                k = 1;
            }
        } break;

        // Cursor navigation within a line
        case 0x9d: if (cursor_col) --cursor_col; k = 1; break;
        case 0x1d: if (cursor_col < scratch_rbuffer.glyph_count) ++cursor_col; k = 1; break;

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
                k = 1;
            }
        } break;
        case 0x91: { // Cursor up
            if (text_line)
            {
                editor_stash_line();
                --text_line;
                editor_fetch_line();
                editor_update_cursor();
                k = 1;
            }
        } break;
        case 0xF5: {
            // XXX - Hide cursor
            // XXX - Backup editor buffer
            // XXX - Render next and previous slides
            copy_trigger_start = slide_start[slide_number];
            copy_trigger_end = slide_start[slide_number+3];
            editor_stash_line();
            editor_save_slide();
            copy_trigger_start = 0;
            copy_trigger_end = 0;
            present_mode = 1;
        } break;
        default: break;
    }
    if (k && cursor_col > 0 && cursor_col <= scratch_rbuffer.glyph_count)
    {
        if (font_id != scratch_rbuffer.glyphs[cursor_col-1].font_id)
            setFont(scratch_rbuffer.glyphs[cursor_col-1].font_id);
        if (text_colour != scratch_rbuffer.glyphs[cursor_col-1].colour_and_attributes)
            text_colour = scratch_rbuffer.glyphs[cursor_col-1].colour_and_attributes;
    }
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
        mod = READ_MOD();
        key = READ_KEY();
        if (key)
        {
            while (READ_KEY())
            {
                READ_KEY() = 1;
                mod |= READ_MOD();
            }

            // Control+SHIFT <0-9> = select font
            if (!present_mode && (key >= 0x21 && key <= 0x29) && (mod & MOD_CTRL))
                setFont(key - 0x21);
            else if (!present_mode && key >= ' ' && key <= 0x7e)
                editor_insert_codepoint(key);
            else
                editor_process_special_key(key);

            editor_update_cursor();

            for (l = 0; l < 25000; ++l)
                continue;
        }
        else
        {
            if (present_mode)
            {
                POKE(0xD015, PEEK(0xD015U) & 0xFE);
            }
            else if (PEEK(0xD012U) > 0xF8)
            {
                if (!(cursor_toggle += 0x10))
                    POKE(0xD015U, (PEEK(0xD015U) ^ 0x01) & 0x0f); // Toggle cursor on/off quickly
            }
        }
    }
}
