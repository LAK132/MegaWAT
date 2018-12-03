#include "fileio.h"

uint8_t file_name[FILE_NAME_MAX_LEN] = "default.mwt";
uint8_t drive = 8;

uint8_t last_message[32];

uint8_t line = 0;
uint32_t sz;
/*
uint8_t rtn1;
uint8_t c;
uint32_t ii;
uint8_t get_error_code(uint8_t LFN)
{
    cbm_k_clrch();
    rtn1 = 0;
    c = '0';
    ii = 0;
    do {
        rtn1 = (rtn1 * 10) + (c - '0');
        cbm_k_chkin(LFN);
        c = cbm_k_basin();
        last_message[ii++] = c;
    } while (c != 0x2C);
    do {
        cbm_k_chkin(LFN);
        c = cbm_k_basin();
        last_message[ii++] = c;
    } while (c != 0x0D);
    last_message[ii] = 0;
    show_last_error();
    return rtn1;
}

uint8_t command[32];
uint8_t drive_command(uint8_t CMD)
{
    for (ii = 0; command[ii] != 0; ++ii)
    {
        cbm_k_ckout(CMD);
        cbm_k_bsout(command[ii]);
    }
    return get_error_code(CMD);
}
// */

FILE *file;
int rtn;
uint8_t ii;
uint32_t jj, diff;

// fread/fwrite are limited to 16 bits,
// so we need a buffer in the 16 pointer range
uint8_t data_buffer[128];

int fileio_save_pres(void)
{
    rtn = 0;
    file = fopen(file_name, "w");
    videoSetSlideMode();
    if (file)
    {
        for (ii = 0; ii < EDITOR_END_SLIDE; ++ii)
        {
            sz = slide_start[ii + 1] - slide_start[ii];
            if (sizeof(sz) != fwrite(&sz, 1, sizeof(sz), file))
            {
                editor_show_message(line++, "failed to write size");
                for (;;) TOGGLE_BACK();
            }
            TOGGLE_BACK();

            if (sizeof(slide_colour[ii]) != fwrite(&slide_colour[ii], 1, sizeof(slide_colour[ii]), file))
            {
                editor_show_message(line++, "failed to write slide_colour");
                for (;;) TOGGLE_BACK();
            }
            TOGGLE_BACK();

            for (jj = 0; jj < sz; jj += sizeof(data_buffer))
            {
                diff = sz - jj;
                if (diff > sizeof(data_buffer))
                    diff = sizeof(data_buffer);
                lcopy(slide_start[ii] + jj, data_buffer, diff);
                if (diff != fwrite(data_buffer, 1, diff, file))
                {
                    editor_show_message(line++, "failed to write slide_start");
                    for (;;) TOGGLE_BACK();
                }
                TOGGLE_BACK();
            }
            TOGGLE_BACK();
        }
        rtn = fclose(file);
    }
    else
    {
        rtn = errno;
        editor_show_message(line++, "failed to open file");
        for (;;) TOGGLE_BACK();
    }
    videoSetSlideMode();

    return rtn;
}

int fileio_load_pres(void)
{
    rtn = 0;
    line = 0;
    file = fopen(file_name, "r");
    videoSetSlideMode();
    if (file)
    {
        slide_start[0] = SLIDE_DATA;
        for (ii = 0; ii < EDITOR_END_SLIDE; ++ii)
        {
            if (sizeof(sz) != fread(&sz, 1, sizeof(sz), file))
            {
                editor_show_message(line++, "failed to read size");
                for (;;) TOGGLE_BACK();
            }
            TOGGLE_BACK();

            slide_start[ii + 1] = slide_start[ii] + sz;
            if (sizeof(slide_colour[ii]) != fread(&slide_colour[ii], 1, sizeof(slide_colour[ii]), file))
            {
                editor_show_message(line++, "failed to read slide_colour");
                for (;;) TOGGLE_BACK();
            }
            TOGGLE_BACK();

            for (jj = 0; jj < sz; jj += sizeof(data_buffer))
            {
                diff = sz - jj;
                if (diff > sizeof(data_buffer))
                    diff = sizeof(data_buffer);
                if (diff != fread(data_buffer, 1, diff, file))
                {
                    editor_show_message(line++, "failed to write slide_start");
                    for (;;) TOGGLE_BACK();
                }
                lcopy(data_buffer, slide_start[ii] + jj, diff);
                TOGGLE_BACK();
            }
            TOGGLE_BACK();
        }
        rtn = fclose(file);
    }
    else
    {
        rtn = errno;
        editor_show_message(line++, "failed to open file");
        for (;;) TOGGLE_BACK();
    }
    videoSetSlideMode();

    return rtn;
}