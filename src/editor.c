#include "editor.h"

unsigned char text_colour = 1;
unsigned char cursor_col = 0;
// Physical rows on the slide (as compared to the lines of text that
// produce them).
unsigned char current_row = 0;
unsigned char next_row = 0;
// Which line of text we are on
unsigned char text_line = 0;
// Rows where each line of text is drawn
unsigned char text_line_first_rows[30];
unsigned char text_line_count = 1;

// XXX For now, have a fixed 128x30 buffer for text
// (We allow 128 instead of 100, so that we can have a number of font and attribute/colour changes
// in there.)
#define EDITOR_LINE_LEN 128
#define EDITOR_MAX_LINES 16
unsigned int editor_buffer[EDITOR_MAX_LINES][EDITOR_LINE_LEN];

char maxlen = 80;
unsigned int key = 0;
char mod = 0;
int xx;
unsigned char h;
uint32_t sram, cram;
uint16_t cursor_toggle = 0;

void editor_insert_line(unsigned char before)
{
    // Insert a new blank line before line #before
}

void editor_initialise(void)
{
    for (y = 0; y < EDITOR_MAX_LINES; ++y)
    {
        lfill((long)&editor_buffer[y][0], 0x00, EDITOR_LINE_LEN * char_size);
        text_line_first_rows[y] = y;
    }
    text_line_count = 0;
}

void editor_stash_line(unsigned char line_num)
{
    // Stash the line encoded in scratch buffer into the specified line

    // Setup default font and attributes, so that we can notice when
    // switching.
    font_id = 0;
    text_colour = 1;

    y = 0;
    for (x = 0; x < scratch_rbuffer.glyph_count; ++x)
    {
        // Make sure we don't overflow
        if (y >= (EDITOR_LINE_LEN - char_size))
            break;

        // Encode any required colour/attribute/font changes
        if (scratch_rbuffer.glyphs[x].colour_and_attributes != text_colour)
        {
            // Colour change, so write 0xe0XX, where XX is the colour
            editor_buffer[line_num][y++] = 0xe000 + scratch_rbuffer.glyphs[x].colour_and_attributes;
            text_colour = scratch_rbuffer.glyphs[x].colour_and_attributes;
        }
        if (scratch_rbuffer.glyphs[x].font_id != font_id)
        {
            editor_buffer[line_num][y++] = scratch_rbuffer.glyphs[x].font_id;
            font_id = scratch_rbuffer.glyphs[x].font_id;
        }

        // Write glyph
        editor_buffer[line_num][y++] = scratch_rbuffer.glyphs[x].code_point;
    }
    // Fill remainder of line with end of line markers
    for (; y < EDITOR_LINE_LEN; ++y)
        editor_buffer[line_num][y] = 0;
}

void editor_fetch_line(unsigned char line_num)
{
    active_rbuffer = &scratch_rbuffer;
    // Fetch the specified line, and pre-render it into scratch
    clearRenderBuffer();
    for (h = 0; editor_buffer[line_num][h] && (h < EDITOR_LINE_LEN); ++h)
        if ((editor_buffer[line_num][h] < 0xe000) || (editor_buffer[line_num][h] >= 0xf800))
            renderGlyph(ASSET_RAM,
                        editor_buffer[line_num][h],
                        text_colour,
                        ATTRIB_ALPHA_BLEND,
                        0xFF // Append to end of line
            );
        else
        {
            // Reserved code points are used to record colour and other attribute changes
            switch (editor_buffer[line_num][h] & 0xff00)
            {
                case 0xe000: {
                    // Set colour and attributes
                    text_colour = editor_buffer[line_num][h] & 0xff;
                } break;
                case 0xe100: {
                    // Set font id
                    font_id = editor_buffer[line_num][h] & 0xff;
                    // XXX - Set active font
                } break;
                default: break;
            }
        }
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
    xx = 5; // Fudge factor
    for (x = 0; x < cursor_col; ++x)
        xx += (scratch_rbuffer.glyphs[x].columns * 8) - scratch_rbuffer.glyphs[x].trim_pixels;
    // Work out cursor height
    // Note: Sprites use V200 dimensions, thus 4 V200 pixels per 8 V400 pixels.
    h = 4 * (scratch_rbuffer.max_above + scratch_rbuffer.max_below);
    if (h < 4)
        h = 4;
    y = 30 + text_line_first_rows[text_line] * 4;
    // Set extended Y height to match required height.
    POKE(0xD056, h);
    // Make cursor be text colour (will alternate to another colour as well)
    POKE(0xD027U, text_colour);
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

void editor_redraw_line(void)
{
    screen_rbuffer.rows_used = current_row;
    outputLineToRenderBuffer();
}

void editor_insert_codepoint(unsigned int code_point)
{
    active_rbuffer = &scratch_rbuffer;
    // Natural key -- insert here
    z = scratch_rbuffer.glyph_count;
    renderGlyph(ASSET_RAM, code_point, text_colour, ATTRIB_ALPHA_BLEND, cursor_col);

    // Check if this code point grew the height of the line.
    // If so, push everything else down before pasting
    if ((text_line + 1) < EDITOR_MAX_LINES)
    {
        // s = glyph height - row height (how much bigger the glyph is than the row)
        s = (scratch_rbuffer.max_above + scratch_rbuffer.max_below) -
            (text_line_first_rows[text_line + 1] - text_line_first_rows[text_line]);
        if (s > 0)
        {
            // Yes, it grew.
            // Adjust first row for all following lines
            for (y = text_line + 1; y < EDITOR_MAX_LINES; ++y)
                text_line_first_rows[y] += s;

            // Now shift everything down (on the SCREEN buffer)
            sram = screen_rbuffer.screen_ram + (screen_size - screen_width);
            cram = screen_rbuffer.colour_ram + (screen_size - screen_width);
            l    = screen_rbuffer.screen_ram + (text_line_first_rows[text_line + 1] * screen_width);
            while (sram >= l)
            {
                lcopy_safe(sram - screen_width, sram, screen_width);
                lcopy_safe(cram - screen_width, cram, screen_width);
                sram -= screen_width;
                cram -= screen_width;
            }
        }
    }

    screen_rbuffer.rows_used = current_row;
    outputLineToRenderBuffer();
    next_row = screen_rbuffer.rows_used;

    // Only advance cursor if the glyph was actually rendered
    if (scratch_rbuffer.glyph_count > z)
        ++cursor_col;
    // if (cursor_col > scratch_rbuffer.glyph_count)
    //     cursor_col = scratch_rbuffer.glyphs; // unsigned char = pointer ??
}

void editor_process_special_key(uint8_t key)
{
    switch (key)
    {
        // Commodore colour selection keys
        // CTRL
        case 0x05: text_colour = (text_colour & 0xF0) | COLOUR_BLACK; break;
        case 0x1C: text_colour = (text_colour & 0xF0) | COLOUR_WHITE; break;
        case 0x9F: text_colour = (text_colour & 0xF0) | COLOUR_RED; break;
        case 0x9C: text_colour = (text_colour & 0xF0) | COLOUR_CYAN; break;
        case 0x1E: text_colour = (text_colour & 0xF0) | COLOUR_PURPLE; break;
        case 0x1F: text_colour = (text_colour & 0xF0) | COLOUR_GREEN; break;
        case 0x9E: text_colour = (text_colour & 0xF0) | COLOUR_BLUE; break;
        case 0x81: text_colour = (text_colour & 0xF0) | COLOUR_YELLOW; break;
        // SUPER
        case 0x95: text_colour = (text_colour & 0xF0) | COLOUR_ORANGE; break;
        case 0x96: text_colour = (text_colour & 0xF0) | COLOUR_BROWN; break;
        case 0x97: text_colour = (text_colour & 0xF0) | COLOUR_PINK; break;
        case 0x98: text_colour = (text_colour & 0xF0) | COLOUR_DARKGREY; break;
        case 0x99: text_colour = (text_colour & 0xF0) | COLOUR_MEDIUMGREY; break;
        case 0x9A: text_colour = (text_colour & 0xF0) | COLOUR_LIGHTGREEN; break;
        case 0x9B: text_colour = (text_colour & 0xF0) | COLOUR_LIGHTBLUE; break;
        // The following need fixes to the ASCII keyboard scanner
        // case 0x9c: text_colour=15; break;
        // case 0x92: text_colour|=ATTRIB_BLINK; break;

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
                deleteGlyph(--cursor_col);
                screen_rbuffer.rows_used = current_row;
                editor_redraw_line();
                if (screen_rbuffer.rows_used < next_row)
                {
                    // Deleting a character reduced the row height,
                    // So shuffle up the rows below, and fill in the
                    // bottom of the screen.

                    // Copy rows up
                    lcopy(screen_rbuffer.screen_ram + next_row * screen_width,
                        screen_rbuffer.screen_ram + (screen_rbuffer.rows_used * screen_width),
                        screen_size - (screen_width * (60 - next_row))
                    );

                    // XXX Fill in bottom of screen

                    // Adjust starting rows for following lines
                    x = next_row - screen_rbuffer.rows_used;
                    for (y = text_line + 1; y < EDITOR_MAX_LINES; y++)
                        text_line_first_rows[y] -= x;
                }
                next_row = screen_rbuffer.rows_used;
            }
            else
            {
                // Backspace from start of line
                // XXX - Should combine remainder of line with previous line (if it will fit)
                // XXX - Shuffle up the rest of the screen,
                // XXX - Select the previous line
                // XXX - Re-render the previous line
                //       (And shuffle everything down in the process if this line grew taller)
            }
        } break;

        // Cursor navigation within a line
        case 0x9d: if (cursor_col) --cursor_col; break;
        case 0x1d: if (cursor_col < scratch_rbuffer.glyph_count) ++cursor_col; break;

        // Cursor navigation between lines
        // Here we adjust which line we are editing,
        // Fix cursor position if it would be beyond the end of the line
        case 0x0d: { // RETURN
            // Break line into two
            editor_insert_line(text_line + 1);
            cursor_col = 0;
        } // don't break
        case 0x11: { // Cursor down
            editor_stash_line(text_line);
            if (text_line < EDITOR_MAX_LINES)
                current_row = text_line_first_rows[++text_line];
            editor_fetch_line(text_line);
            editor_update_cursor();
            editor_redraw_line();
        } break;
        case 0x91: { // Cursor up
            editor_stash_line(text_line);
            if (text_line)
                current_row = text_line_first_rows[--text_line];
            editor_fetch_line(text_line);
            editor_update_cursor();
            editor_redraw_line();
        } break;
        default: break;
    }
}

void editor_poll_keyboard(void)
{
    while (key != KEY_ESC)
    {
        mod = READ_MOD();
        key = READ_KEY();
        if (key)
        {
            while (READ_KEY())
            {
                READ_KEY() = 1;
                mod |= READ_MOD();
            }

            if (key >= ' ' && key <= 0x7e)
                editor_insert_codepoint(key);
            else
                editor_process_special_key(key);

            editor_update_cursor();

            for (l = 0; l < 25000; ++l)
                continue;
        }
        else if (PEEK(0xD012U) > 0xF8)
            if (!(cursor_toggle += 0x10))
                POKE(0xD015U, (PEEK(0xD015U) ^ 0x01) & 0x0f); // Toggle cursor on/off quickly
    }
}
