#include "fileio.h"

uint8_t file_name[FILE_NAME_MAX_LEN];

FILE *file;
int rtn;
int fileio_create_pres(void)
{
    rtn = 0;
    file = fopen(file_name, "\x77\x62"); // "wb"
    if (file)
    {
        fclose(file);
    }
    else rtn = errno;
    return rtn;
}

int fileio_load_pres(void)
{
    rtn = 0;
    file = fopen(file_name, "\x72\x62"); // "rb"
    if (file)
    {
        fclose(file);
    }
    else rtn = errno;
    return rtn;
}

int fileio_save_pres(void)
{
    rtn = 0;
    file = fopen(file_name, "\x72\x2B\x62"); // "r+b"
    if (file)
    {
        fclose(file);
    }
    else rtn = errno;
    return rtn;
}