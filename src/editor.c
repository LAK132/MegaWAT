#include "editor.h"

uint8_t active_slide;
uint8_t present_mode = 0;
uint8_t text_colour = 1;
uint8_t cursor_col = 0;
// Which line of text we are on
uint8_t text_line = 0;
// Rows where each line of text is drawn
uint16_t text_line_start[EDITOR_MAX_LINES];
uint16_t text_line_first_rows[EDITOR_MAX_LINES];

// Physical rows on the slide (as compared to the lines of text that
// produce them).
#define CURRENT_ROW (text_line_first_rows[text_line])
#define NEXT_ROW (text_line_first_rows[text_line + 1])
#define END_ROW (text_line_first_rows[EDITOR_END_LINE])
#define CURRENT_ROW_HEIGHT (NEXT_ROW - CURRENT_ROW)

uint16_t _editor_scratch_slide[SLIDE_SIZE / sizeof(uint16_t)];
string_t scratch_slide;

string_t presentation;
uint16_t slide_start[EDITOR_MAX_SLIDES];

uint8_t slide_number = 0;
uint8_t preload_slide_number[3] = {0};
uint8_t slide_colour[EDITOR_MAX_SLIDES];
uint8_t slide_resolution[EDITOR_MAX_SLIDES];
uint8_t slide_font_pack[EDITOR_MAX_SLIDES][FILE_NAME_MAX_LEN-6];

#define HIDE_CURSOR() HIDE_SPRITE(0)
#define SHOW_CURSOR() SHOW_SPRITE(0)
#define TOGGLE_CURSOR() TOGGLE_SPRITE(0)

int8_t maxlen = 80;
uint8_t key = 0;
uint8_t mod = 0;
uint32_t sram, cram;
uint16_t cursor_toggle = 0;

#define editor_hide_slide_number() lfill(0x0500,0x00,0x100)

void editor_get_line_info(void)
{
    static uint8_t y;
    static uint16_t l;

    y = 0;
    text_line_start[y++] = 0;
    for (l = 0; y < EDITOR_MAX_LINES && l * sizeof(uint16_t) < scratch_slide.size; ++l)
        if (string_peek(scratch_slide, l) == 0)
            text_line_start[y++] = l + 1;

    for (; y < EDITOR_MAX_LINES; ++y)
        text_line_start[y] = text_line_start[y-1] + 1;

    for (y = 0; y < EDITOR_MAX_LINES; ++y)
        text_line_first_rows[y] = y;
}

void editor_initialise(void)
{
    videoSetSlideMode();

    // Fill the rest of the buffer with zeros
    string_clear(scratch_slide);

    editor_get_line_info();

    text_line = 0;
    cursor_col = 0;

    lfill(data_buffer, 0, sizeof(data_buffer));
    lfill(preload_slide_number, EDITOR_MAX_SLIDES, sizeof(preload_slide_number));
    lcopy(slide_font_pack[0], data_buffer, strlen(slide_font_pack[slide_number]));
    if (slide_font_pack[0][0] != 0)
    {
        if (fileio_load_font())
        {
            lfill(&(slide_font_pack[slide_number][0]), 0,
                sizeof(slide_font_pack[slide_number]));
            for (READ_KEY() = 1; READ_KEY() != KEY_RETURN; TOGGLE_BACK()) TOGGLE_BACK();
        }
    }

    active_slide = 0;
    slide_number = 0;
    // use smart load to make sure slide 2 is preloaded correctly
    editor_smart_load_slide(0, 1, 1);

    editor_show_slide_number();
}

void editor_start(void)
{
    static uint8_t y;

    videoSetSlideMode();

    scratch_slide.ptr = (longptr_t)_editor_scratch_slide;
    scratch_slide.size = sizeof(_editor_scratch_slide);
    string_clear(scratch_slide);

    presentation.ptr = (longptr_t)SLIDE_DATA;
    presentation.size = SLIDE_DATA_SIZE;
    string_clear(presentation);

    lfill(SLIDE_DATA, 0, SLIDE_DATA_SIZE);
    slide_start[0] = 0;
    lfill(slide_font_pack[0], 0, sizeof(slide_font_pack[0]));
    for (y = 1; y < EDITOR_MAX_SLIDES; ++y)
    {
        slide_start[y] = slide_start[y-1] + EDITOR_END_LINE;
        lfill(slide_font_pack[y], 0, sizeof(slide_font_pack[y]));
    }

    // default to no file_name
    lfill((ptr_t)file_name, 0, sizeof(file_name));

    // default slide colours to 0x6
    lfill((ptr_t)slide_colour, 0x6, sizeof(slide_colour));

    editor_show_cursor();

    editor_initialise();
}

const string_t scratch_line = { 0x1F770, 0x20000 - 0x1F770 };
void editor_stash_line(void)
{
    static uint8_t x, y, z, c;

    // Stash the line encoded in scratch buffer into the specified line

    string_clear(scratch_line);

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
                string_poke(scratch_line, y, 0xE000 | (scratch_rbuffer.glyphs[x].colour_and_attributes & 0xFF));
                ++y;
                text_colour = scratch_rbuffer.glyphs[x].colour_and_attributes;
                if (y >= EDITOR_LINE_LEN - 2) break;
            }
            if (x == 0 || scratch_rbuffer.glyphs[x].font_id != font_id)
            {
                string_poke(scratch_line, y, 0xE100 | (scratch_rbuffer.glyphs[x].font_id & 0xFF));
                ++y;
                font_id = scratch_rbuffer.glyphs[x].font_id;
                if (y >= EDITOR_LINE_LEN - 2) break;
            }

            // Write glyph
            string_poke(scratch_line, y, scratch_rbuffer.glyphs[x].code_point);
            ++y;
        }
        // Add EOL to scratch
        string_poke(scratch_line, y, 0); // y is now the length of scratch used
        ++y;

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
            string_remove(&scratch_slide, text_line_start[text_line], x);

            for (; c < EDITOR_MAX_LINES; ++c)
                text_line_start[c] -= x;
        }
        else if (y > z)
        {
            // Space available is smaller than space needed, expand
            x = y - z; // amout to expand by
            string_insert(&scratch_slide, text_line_start[text_line], x);

            for (; c < EDITOR_MAX_LINES; ++c)
                text_line_start[c] += x;
        }

        // copy scratch into main buffer
        string_write_string(scratch_slide, text_line_start[text_line], &scratch_line);
    }
}

void editor_check_line_grew(void)
{
    static int8_t s;
    static uint8_t y;
    static uint32_t l;

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

void editor_check_line_shrunk(void)
{
    static int8_t s;
    static uint8_t y;
    static uint32_t l;

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

            if (END_ROW < EDITOR_END_LINE)
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

char editor_render_glyph(uint16_t code_point)
{
    static char rtn;
    active_rbuffer = &scratch_rbuffer;
    active_glyph = &glyph_buffer;
    getGlyphDetails(code_point, text_colour, 0);
    rtn = renderGlyphDetails(ATTRIB_ALPHA_BLEND, cursor_col);
    editor_check_line_grew();
    return rtn;
}

uint8_t *string_buffer;
void editor_render_string(void)
{
    for (; *string_buffer != 0; ++string_buffer)
    {
        editor_render_glyph(*string_buffer);
        ++cursor_col;
    }
}

void editor_clear_line(void)
{
    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();
    cursor_col = 0;

    editor_check_line_shrunk();
}

void editor_delete_glyph(void)
{
    active_rbuffer = &scratch_rbuffer;
    deleteGlyph(cursor_col);

    editor_check_line_shrunk();
}

void editor_fetch_line(void)
{
    static uint8_t c;
    static uint16_t k, m, code;

    // Fetch the specified line, and pre-render it into scratch

    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();

    // Get start of line
    m = text_line_start[text_line];
    if (m <= scratch_slide.size)
    {
        // Get the start of the next line
        k = text_line_start[text_line + 1];
        // Check if it's past the end of the buffer
        if (k > scratch_slide.size) k = scratch_slide.size;

        c = cursor_col;
        cursor_col = 0xFF;
        code = string_peek(scratch_slide, m);
        while (code && m < k)
        {
            // Reserved code points are used to record colour and other attribute changes
            switch (code & 0xFF00)
            {
                case 0xE000: {
                    // Set colour and attributes
                    text_colour = code & 0xFF;
                } break;
                case 0xE100: {
                    setFont(code & 0xFF);
                } break;
                default: {
                    // if this is actually a code point, render the relevant glyph
                    if (code < 0xD800 || code >= 0xF800)
                        editor_render_glyph(code);
                } break;
            }
            ++m;
            code = string_peek(scratch_slide, m);
        }
    }
    screen_rbuffer.rows_used = CURRENT_ROW;
    outputLineToRenderBuffer();
    cursor_col = c;
}

void editor_show_cursor(void)
{
    const static int16_t x = 5;
    const static uint8_t y = 30;
    // Put cursor in initial position
    POKE(0xD000, x & 0xFF);
    POKE(0xD001, y);
    if (x & 0x100)
        POKE(0xD010U, 0x01);
    else
        POKE(0xD010U, 0);
    if (x & 0x200)
        POKE(0xD05FU, PEEK(0xD05FU)|0x01);
    else
        POKE(0xD05FU, PEEK(0xD05FU)&0xfe);
}

void editor_update_cursor(void)
{
    static int16_t xx;
    static uint8_t x, y, h;

    if (cursor_col > scratch_rbuffer.glyph_count)
        cursor_col = scratch_rbuffer.glyph_count;

    // Work out where cursor should be
    xx = 40-2; // Fudge factor
    for (x = 0; x < cursor_col; ++x)
        xx += (scratch_rbuffer.glyphs[x].size.columns * 8) - scratch_rbuffer.glyphs[x].size.trim_pixels;
    // Work out cursor height
    // Note: Sprites use V200 dimensions, thus 4 V200 pixels per 8 V400 pixels.
    h = 4 * (scratch_rbuffer.max_above + scratch_rbuffer.max_below);
    if (h < 4)
        h = 4;
    y = 39 + text_line_first_rows[text_line] * 4;
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

// NOTE: we don't add the code_point into the line buffer until we stash,
// this allows us to prevent adding redundant modifier codes (ie font select codes)

char editor_insert_codepoint(uint16_t code_point)
{
    static char rtn;
    static uint8_t z;

    // Shift to fix ASCII vs PETSCII for the C64 font
    if (!font_id && code_point >= 0x60)
        code_point -= 0x60;

    z = scratch_rbuffer.glyph_count;
    rtn = editor_render_glyph(code_point);
    screen_rbuffer.rows_used = CURRENT_ROW;
    outputLineToRenderBuffer();

    // Only advance cursor and save code point if the glyph was actually rendered
    if (scratch_rbuffer.glyph_count > z)
        ++cursor_col;
    if (cursor_col > scratch_rbuffer.glyph_count)
        cursor_col = scratch_rbuffer.glyph_count;
    return rtn;
}

void editor_save_slide(void)
{
    static uint8_t c;
    static uint16_t i, j, l;

    l = text_line_start[EDITOR_END_LINE] - text_line_start[0];

    // We use slide_number + 1 lots, so just do the calculation once
    c = slide_number + 1;

    // Find how long this line is currently in the buffer
    // +1 is safe here because slide_start is oversized by 1
    j = slide_start[c] - slide_start[slide_number];

    // make sure we have enough space to stash the current slide
    if (j > l)
    {
        // Space available is bigger than space needed, shrink
        i = j - l; // amount to shrink by

        string_remove(&presentation, slide_start[slide_number], i);

        for (; c < EDITOR_MAX_SLIDES; ++c)
            slide_start[c] -= i;
    }
    else if (l > j)
    {
        // Space available is smaller than space needed, expand
        i = l - j; // amout to expand by

        string_insert(&presentation, slide_start[slide_number], i);

        for (; c < EDITOR_MAX_SLIDES; ++c)
            slide_start[c] += i;
    }
    // copy scratch into main buffer
    // can't use string_write_string as scratch_slide isn't null terminated
    lcopy(scratch_slide.ptr, string_loff(presentation, slide_start[slide_number]),
        l * sizeof(uint16_t));
}

void editor_refresh_slide(void)
{
    static uint8_t w;

    editor_get_line_info();

    active_rbuffer = &scratch_rbuffer;
    clearRenderBuffer();
    active_rbuffer = &screen_rbuffer;
    clearRenderBuffer();

    w = text_line;
    for (text_line = 0; text_line < EDITOR_END_LINE; ++text_line)
    {
        editor_fetch_line();
        editor_stash_line();
    }
    text_line = w;
}

void editor_load_slide(void)
{
    static uint16_t j;

    string_clear(scratch_slide);

    // check if we need to trim a little off the end of the slide to get it to fit in the buffer
    j = slide_start[slide_number + 1] - slide_start[slide_number];
    if (j > scratch_slide.size)
        j -= scratch_slide.size;
    else
        j = 0;

    // Copy the next slides buffer into editor buffer
    string_copy_substring(scratch_slide, presentation, slide_start[slide_number], slide_start[slide_number + 1] - j);

    // Update line starts for new buffer
    editor_refresh_slide();
    editor_update_cursor();
}

void editor_insert_line(void)
{
    static uint8_t c, i, w;
    static uint16_t code;
    if (text_line_start[EDITOR_END_LINE] * sizeof(uint16_t) >= scratch_slide.size ||
        text_line_start[EDITOR_END_LINE-1]+1 != text_line_start[EDITOR_END_LINE])
        return; // no room to instert a new line

    // insert break at cursor
    w = text_line_start[text_line + 1] - text_line_start[text_line];
    c = 0;
    for (i = 0; c < cursor_col && i < w; ++i)
    {
        code = string_peek(scratch_slide, text_line_start[text_line] + i);
        if (code < 0xD800 || code >= 0xF800) ++c;
    }

    string_insert_code(&scratch_slide, text_line_start[text_line] + i, 0);

    // refreshing here seems to prevent font and attribute information from messing up.
    // because it bleeds from the previous line?
    editor_refresh_slide();
}

char editor_load_font_pack(uint8_t slide)
{
    if (slide_font_pack[slide][0] != 0)
    {
        // this slide has a set font, load it
        lfill(data_buffer, 0, sizeof(data_buffer));
        lcopy(slide_font_pack[slide], data_buffer,
            sizeof(slide_font_pack[slide]));
        if (fileio_load_font())
        {
            lfill(slide_font_pack[slide], 0,
                sizeof(slide_font_pack[slide]));
            for (READ_KEY() = 1; READ_KEY() != KEY_RETURN; TOGGLE_BACK()) TOGGLE_BACK();
        }
    }
}

void editor_smart_load_slide(uint8_t curr, uint8_t next, char preload)
{
    // Change the screen address
    videoSetActiveGraphicsBuffer(active_slide);
    // Change the renderer address
    videoSetActiveRenderBuffer(active_slide);
    // Set the border colour
    POKE(0xD020, slide_colour[curr]);
    // Set the background colour
    POKE(0xD021, slide_colour[curr]);
    // strncmp return 0 if one one of the args is zero length
    if (strncmp(slide_font_pack[slide_number], slide_font_pack[curr], sizeof(slide_font_pack[0])) != 0)
    {
        BLANK_SCREEN();
        // font pack differs from previous slide, must update
        editor_load_font_pack(curr);
        UNBLANK_SCREEN();
    }
    slide_number = curr;
    if (!preload || preload_slide_number[active_slide] != curr)
    {
        BLANK_SCREEN();
        // this slide has not been preloaded, must update
        editor_load_slide();
        preload_slide_number[active_slide] = curr;
        UNBLANK_SCREEN();
    }
    // memcmp does NOT return 0 if one of the args is zero length
    if (preload && memcmp(slide_font_pack[curr], slide_font_pack[next], sizeof(slide_font_pack[0])) == 0)
    {
        // same font, we can safely preload the next slide
        do {
            if (next > curr && preload_slide_number[active_slide ? 0 : 1] != next)
            {
                // next slide should be in a main slide buffer
                videoSetActiveRenderBuffer(active_slide ? 0 : 1);
                preload_slide_number[active_slide ? 0 : 1] = next;
            }
            else if (next < curr && preload_slide_number[2] != next)
            {
                // next slide should be in the secondary slide buffer
                videoSetActiveRenderBuffer(2);
                preload_slide_number[2] = next;
            }
            else break;
            slide_number = next;
            editor_load_slide();

            // set back to current slide
            videoSetActiveRenderBuffer(active_slide);
            slide_number = curr;
        } while (0);
    }
    // Set the border colour again just to be sure
    POKE(0xD020, slide_colour[curr]);
    // Set the background colour again just to be sure
    POKE(0xD021, slide_colour[curr]);
}

void editor_next_slide(char preload)
{
    if (slide_number + 1 < EDITOR_END_SLIDE)
    {
        preload_slide_number[2] = preload_slide_number[active_slide];

        // Switch active slide (next slide)
        if (active_slide) // SLIDE1 active
            active_slide = 0;
        else // SLIDE0 active
            active_slide = 1;

        // Move current slide (SLIDE0/1) to previous slide (SLIDE2)
        lcopy(active_slide ? SLIDE0_SCREEN_RAM : SLIDE1_SCREEN_RAM, SLIDE2_SCREEN_RAM, SLIDE_SIZE);
        lcopy(active_slide ? SLIDE0_COLOUR_RAM : SLIDE1_COLOUR_RAM, SLIDE2_COLOUR_RAM, SLIDE_SIZE);

        // we can safely ignore overflow here
        editor_smart_load_slide(slide_number + 1, slide_number + 2,
            slide_number < (sizeof(slide_number) * 0x100) - 2 ? preload : 0);
    }
}

void editor_previous_slide(char preload)
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

        preload_slide_number[active_slide] = preload_slide_number[2];

        // we can safely ignore overflow here
        editor_smart_load_slide(slide_number - 1, slide_number - 2, slide_number >= 2 ? preload : 0);
    }
}

static void editor_goto_slide(uint8_t num, char preload)
{
    if (slide_number == num)
        return;

    if (slide_number > num)
        while (slide_number > num)
            editor_previous_slide(slide_number - 1 == num ? preload : 0);
    else
        while(slide_number < num)
            editor_next_slide(slide_number + 1 == num ? preload : 0);
}

void editor_show_message(uint8_t line, uint8_t *str)
{
    text_line = line;
    editor_clear_line();
    setFont(0);
    string_buffer = str;
    editor_render_string();
    screen_rbuffer.rows_used = CURRENT_ROW;
    outputLineToRenderBuffer();
}

void editor_process_special_key(uint8_t key)
{
    static uint8_t k, c, i, j;

    k = 0; // if the cursor was moved
    if (present_mode) switch (key)
    {
        case 0x62: TOGGLE_BLANK_SCREEN(); break;
        case 0x0D:
        case 0x20:
        case 0x11:
        case 0x1D: { // next slide
            editor_next_slide(1);
            editor_show_slide_number();
        } break;
        case 0x91:
        case 0x9D: { // previous slide
            editor_previous_slide(1);
            editor_show_slide_number();
        } break;
        case 0x03:
        case 0xF5: {
            UNBLANK_SCREEN(); // just to be sure
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
                k = 1;
            }
            // else if (current_row && text_line)
            else if (text_line)
            {
                // XXX - Should check if the line will fit correctly
                editor_stash_line();

                // move the cursor to the end of the previous line so it's in the correct
                // spot before with append the current line to the end
                --text_line;
                editor_fetch_line();
                cursor_col = 0xFF;
                editor_update_cursor();
                editor_stash_line();
                ++text_line;

                if (text_line < EDITOR_END_LINE)
                {
                    string_remove(&scratch_slide, text_line_start[text_line] - 1, 1);
                    editor_refresh_slide();
                }

                --text_line;
                editor_fetch_line();

                editor_update_cursor();
                k = 1;
            }
        } break;

        // Home
        case 0x13: cursor_col = 0x00; k = 1; break;
        // End (SHIFT + Home)
        case 0x93: cursor_col = 0xFF; k = 1; break;

        // Cursor navigation within a line
        case 0x9d: {
            if (cursor_col)
            {
                --cursor_col;
                k = 1;
            }
            else if (text_line)
            {
                // reached start of line, go up a line
                cursor_col = 0xFF;
                k = 1;
                key = 0x91;
                goto label_change_line;
            }
        } break;
        case 0x1d: {
            if (cursor_col < scratch_rbuffer.glyph_count)
            {
                ++cursor_col;
                k = 1;
            }
            else if (text_line < EDITOR_MAX_LINES)
            {
                // reached end of line, go down a line
                cursor_col = 0;
                k = 1;
                key = 0x11;
                goto label_change_line;
            }
        } break;

        // Cursor navigation between lines
        // Here we adjust which line we are editing,
        // Fix cursor position if it would be beyond the end of the line
        case 0x0d: { // RETURN
            if (text_line < EDITOR_MAX_LINES)
            {
                // Break line into two
                editor_stash_line();
                editor_insert_line();
                ++text_line;
                cursor_col = 0;
                editor_fetch_line();
                editor_update_cursor();
                k = 1;
            }
        } break;
        case 0x11:
        case 0x91: { // change line
            if ((key == 0x11 && text_line < EDITOR_MAX_LINES) ||
                (key == 0x91 && text_line))
            {
                label_change_line:
                editor_stash_line();
                if (key == 0x11)
                    ++text_line;
                else
                    --text_line;
                editor_fetch_line();
                editor_update_cursor();
                k = 1;
            }
        } break;
        case 0xED:
        case 0xEE: { // change slide
            editor_stash_line();
            editor_save_slide();
            if (key == 0xC2 || ((key == 0xED || key == 0xEE) && (mod & MOD_SHIFT)))
                editor_previous_slide(0);
            else
                editor_next_slide(0);
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
            HIDE_CURSOR(); // XXX - show the cursor in the "text box"
            editor_stash_line();
            editor_save_slide();

            j = text_line;
            c = cursor_col;
            // XXX - "Are you sure?" prompt

            active_rbuffer = &screen_rbuffer;
            clearRenderBuffer();
            lfill(preload_slide_number, EDITOR_MAX_SLIDES, sizeof(preload_slide_number));

            editor_show_message(0, "open a different presentation? unsaved changes will be lost");
            editor_show_message(1, file_name);
            editor_show_message(2, "RETURN: ok");
            editor_show_message(3, "ESC: cancel");

            i = strlen(file_name);

            for (key = READ_KEY(); key != KEY_ESC && key != KEY_RETURN; key = READ_KEY())
            {
                if (((key >= 0x30 && key <= 0x39) || (key >= 0x41 && key <= 0x5A) ||
                    (key >= 0x61 && key <= 0x7A)) && i < sizeof(file_name)-6)
                {
                    if (key >= 0x61 && key <= 0x7A) key -= 0x20;
                    file_name[i] = key;
                    ++i;
                    editor_show_message(1, file_name);
                    READ_KEY() = 1;
                }
                else if (key == 0x14 && i > 0)
                {
                    --i;
                    file_name[i] = 0;
                    editor_show_message(1, file_name);
                    READ_KEY() = 1;
                }
            }
            if (key == KEY_RETURN)
            {
                // XXX - use slide to show SD card contents
                // XXX - use hardware reverse to show selection
                // XXX - on RETURN: load presentation
                // XXX - on ESC: return to editing previous presentation
                fileio_load_pres();
                editor_initialise();
            }
            else
            {
                // return to normal editing
                text_line = j;
                cursor_col = c;
            }

            editor_load_slide();
            editor_fetch_line();
            SHOW_CURSOR();
            READ_KEY() = 0;
            k = 1;
        } break;
        case 0xD3: { // MEGA S
            HIDE_CURSOR(); // XXX - show the cursor in the "text box"
            editor_stash_line();
            editor_save_slide();
            j = text_line;
            c = cursor_col;
            do
            {
                // if not saved previously or SHIFT held, go to SAVE AS screen
                if (file_name[0] == 0 || mod & MOD_SHIFT)
                {
                    // Save As
                    // XXX - switch to blank slide
                    // XXX - use scratch buffer for input
                    // XXX - on RETURN: save
                    // XXX - on ESC: cancel

                    active_rbuffer = &screen_rbuffer;
                    clearRenderBuffer();

                    i = strlen(file_name);

                    editor_show_message(0, "save as");
                    editor_show_message(1, file_name);
                    editor_show_message(2, "RETURN: ok");
                    editor_show_message(3, "ESC: cancel");

                    for (key = READ_KEY(); key != KEY_ESC && key != KEY_RETURN; key = READ_KEY())
                    {
                        if (((key >= 0x30 && key <= 0x39) || (key >= 0x41 && key <= 0x5A) ||
                            (key >= 0x61 && key <= 0x7A)) && i < sizeof(file_name)-6)
                        {
                            if (key >= 0x61 && key <= 0x7A) key -= 0x20;
                            file_name[i] = key;
                            ++i;
                            editor_show_message(1, file_name);
                            READ_KEY() = 1;
                        }
                        else if (key == 0x14 && i > 0)
                        {
                            --i;
                            file_name[i] = 0;
                            editor_show_message(1, file_name);
                            READ_KEY() = 1;
                        }
                    }
                    if (key != KEY_RETURN)
                    {
                        // clear out filename so we don't accidentally
                        // save with a bad name later
                        lfill(file_name, 0, sizeof(file_name));
                        break; // we don't want to save
                    }
                }
                if (fileio_save_pres())
                    lfill(file_name, 0, sizeof(file_name));
            } while (0);
            lfill(preload_slide_number, EDITOR_MAX_SLIDES, sizeof(preload_slide_number));
            text_line = j;
            cursor_col = c;
            editor_load_slide();
            editor_fetch_line();
            SHOW_CURSOR();
            READ_KEY() = 0;
            k = 1;
        } break;
        case 0xCE: { // MEGA N
            // New
            HIDE_CURSOR(); // XXX - show the cursor in the "text box"
            editor_stash_line();
            editor_save_slide();
            j = text_line;
            c = cursor_col;
            // XXX - "Are you sure?" prompt

            active_rbuffer = &screen_rbuffer;
            clearRenderBuffer();
            lfill(preload_slide_number, EDITOR_MAX_SLIDES, sizeof(preload_slide_number));

            editor_show_message(0, "start a new presentation? unsaved changes will be lost");
            editor_show_message(1, "RETURN: ok");
            editor_show_message(2, "ESC: cancel");

            while (READ_KEY() != KEY_ESC && READ_KEY() != KEY_RETURN) continue;
            if (READ_KEY() == KEY_RETURN)
            {
                // clear out filename so we don't accidentally
                // save with a bad name later
                lfill(file_name, 0, sizeof(file_name));
                // restart
                editor_start();
            }
            else
            {
                // return to normal editing
                text_line = j;
                cursor_col = c;
            }
            editor_load_slide();
            editor_fetch_line();
            SHOW_CURSOR();
            READ_KEY() = 0;
            k = 1;
        } break;
        case 0xC6: { // MEGA F
            HIDE_CURSOR(); // XXX - show the cursor in the "text box"
            editor_stash_line();
            editor_save_slide();
            j = text_line;
            c = cursor_col;
            // XXX - "Are you sure?" prompt

            active_rbuffer = &screen_rbuffer;
            clearRenderBuffer();
            lfill(preload_slide_number, EDITOR_MAX_SLIDES, sizeof(preload_slide_number));

            lfill(data_buffer, 0, sizeof(data_buffer));
            i = strlen(slide_font_pack[slide_number]);
            lcopy(slide_font_pack[slide_number], data_buffer, i);

            editor_show_message(0, "set font pack for this slide");
            editor_show_message(1, data_buffer);
            editor_show_message(2, "RETURN: ok");
            editor_show_message(3, "ESC: cancel");

            for (key = READ_KEY(); key != KEY_ESC && key != KEY_RETURN; key = READ_KEY())
            {
                if (((key >= 0x30 && key <= 0x39) || (key >= 0x41 && key <= 0x5A) ||
                    (key >= 0x61 && key <= 0x7A)) && i < sizeof(slide_font_pack[slide_number]))
                {
                    if (key >= 0x61 && key <= 0x7A) key -= 0x20;
                    data_buffer[i] = key;
                    ++i;
                    editor_show_message(1, data_buffer);
                    READ_KEY() = 1;
                }
                else if (key == 0x14 && i > 0)
                {
                    --i;
                    data_buffer[i] = 0;
                    editor_show_message(1, data_buffer);
                    READ_KEY() = 1;
                }
            }
            if (key == KEY_RETURN)
            {
                lfill(slide_font_pack[slide_number], 0, sizeof(slide_font_pack[slide_number]));
                if (fileio_load_font())
                    for (READ_KEY() = 1; READ_KEY() != KEY_RETURN; TOGGLE_BACK()) TOGGLE_BACK();
                else
                    lcopy(data_buffer, slide_font_pack[slide_number], i);
            }
            editor_load_slide();
            editor_fetch_line();
            text_line = j;
            cursor_col = c;
            READ_KEY() = 1;
            READ_MOD() = 1;
            SHOW_CURSOR();
            READ_KEY() = 0;
            k = 1;
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

uint8_t remainder,yy;
uint8_t slide_num_message[8]={19+64,12,9,4,5,' ','0','0'}; // "Slide 00"
uint8_t edit_mode_message[8]={5+64,4,9,20,9,14,7,'.'};
uint8_t present_mode_message[8]={16+64,18,5,19,5,14,20,'.'};

void editor_show_slide_number(void)
{
    // Clear sprite (8x21 bytes) and write
    // info in it
    lfill(0x0500, 0x00, 0x100);

    // Produce message to show on the sprite
    slide_num_message[6] = '0';
    remainder = slide_number + 1;
    while (remainder > 9)
    {
        remainder -= 10;
        ++(slide_num_message[6]);
    }
    slide_num_message[7] = 0x30 + remainder;
    if (slide_num_message[6] == '0')
    {
        slide_num_message[6] = slide_num_message[7];
        slide_num_message[7] = ' ';
    }
    for (remainder = 0; remainder < 8; ++remainder)
        for (yy = 0; yy < 8; ++yy)
            POKE(0x0500 + remainder + (yy * 8), lpeek(0x2D800 + (slide_num_message[remainder] * 8) + yy));

    if (present_mode)
        for (remainder = 0; remainder < 8; ++remainder)
            for (yy = 0; yy < 8; ++yy)
                POKE(0x0540 + remainder + (yy * 8), lpeek(0x2D800 + (present_mode_message[remainder] * 8) + yy));
    else // Display editing in reverse text, so that it is more obvious
        for(remainder = 0; remainder < 8; ++remainder)
            for(yy = 0; yy < 8; ++yy)
                POKE(0x0540 + remainder + (yy * 8), lpeek(0x2DC00 + (edit_mode_message[remainder] * 8) + yy));

    // Set sprite data fetch area to $0500
    POKE(2041, 0x0500 / 0x40);
    // Extended width (64 pixels wide)
    POKE(0xD057U, 0x02);

    // Position sprite 2 near lower right corner
    POKE(0xD003U, 0xf7);
    POKE(0xD002U, 0x76);
    POKE(0xD010U, 0x00);
    POKE(0xD05FU, PEEK(0xD05FU) | 0x02);

    // Make sprite expanded in X any Y
    // POKE(0xD017U, PEEK(0xD017U) | 2);
    POKE(0xD01DU, PEEK(0xD01DU) | 2);

    // Set sprite colour to light green
    POKE(0xD028U, 0x0D);

    // Make sprite 2 visible again
    POKE(0xD015U, PEEK(0xD015U) | 0x02);

    // Start with message fully faded in
    POKE(0xD074U, 0x02); // Make sprite alpha blended
    POKE(0xD075U, 0xFF); // Alpha blend set to fully visible
}

unsigned char last_joystick=0;
void editor_poll_keyboard(void)
{
    static uint16_t l;

    #ifndef __MEGA65__
    while (key != KEY_ESC)
    #else
    for (;;)
    #endif
    {
        // TOGGLE_BACK();
        key = READ_KEY();
	if (!key) {
	  // No key pressed, so check joystick.
	  // Fire = SPACE to advance slides in presentation mode
	  // Left/right/up/down direction keys. This will allow navigation via joystick in
	  // both editor and slide-show mode
	  l=PEEK(56320U);
	  if ((!present_mode)||(last_joystick!=l)) {
	    // Had to comment these out to make program fit in memory! so only left and right
	    //	    if (!(l&1)) key=0x91;
	    //	    if (!(l&2)) key=0x11;
	    if (!(l&4)) key=0x9d;
	    if (!(l&8)) key=0x1d;
	  }
	  if (present_mode) { if (!(l&10)) key=0x20; }
	  last_joystick=l;
	}
        if (key)
        {
            // Allow enough time for modifier flags to get asserted
            mod = READ_MOD();
            for (l = 0; l < 3000; ++l)
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
            {
                setFont(key - 0x21);
            }
            else
            {
                if (!present_mode && key >= ' ' && key <= 0x7e)
                {
                    editor_insert_codepoint(key);
                }
                else
                {
                    editor_process_special_key(key);
                }

                editor_update_cursor();

                // Make sure cursor is on when typing
                SHOW_CURSOR();
            }

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
