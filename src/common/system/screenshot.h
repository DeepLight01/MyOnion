#ifndef SCREENSHOT_H__
#define SCREENSHOT_H__

#include <stdio.h>
#include <png/png.h>

#include "utils/utils.h"
#include "utils/process.h"
#include "utils/log.h"
#include "display.h"

//
//    [onion] get recent filename from content_history.lpl
//
char* getrecent_onion(char *filename) {
    FILE *fp;
    char key[256], val[256];
    char *valptr, *strptr;
    int f;

    *filename = 0;
    if ( (fp = fopen("/mnt/SDCARD/Saves/CurrentProfile/lists/content_history.lpl", "r")) ) {
        key[0] = 0; val[0] = 0;
        while ((f = fscanf(fp, "%255[^:]:%255[^\n]\n", key, val)) != EOF) {
            if (!f) { if (fscanf(fp, "%*[^\n]\n") == EOF) break; else continue; }
            if (str_trim(key, 256, key, false) && str_trim(val, 256, val, true)) {
                if (!strcmp(key, "\"path\"") && (valptr = strrchr(val, '/'))) {
                    valptr++;
                    if ((strptr = strrchr(valptr, '"'))) *strptr = 0;
                    strcpy(filename, valptr);
                    break;
                }
            }
            key[0] = 0; val[0] = 0;
        }
        fclose(fp);
    }

    printf_debug("Last game: %s\n", filename);

    if (*filename == 0) return NULL;
    return filename;
}

//
//    Get most recent file name for screenshot
//
char* getrecent_png(char *filename) {
    FILE        *fp;
    char        *fnptr,    *strptr;
    uint32_t    i;

    strcpy(filename, "/mnt/SDCARD/Screenshots/");
    if (!file_exists(filename)) mkdir(filename, 777);

    fnptr = filename + strlen(filename);

    if (file_exists("/mnt/SDCARD/.tmp_update/cmd_to_run.sh"))
        getrecent_onion(fnptr);

    if (!(*fnptr)) {
        if (process_searchpid("gameSwitcher")) strcat(filename, "gameSwitcher");
        else strcat(filename, "MainUI");
    }

    fnptr = filename + strlen(filename);
    for (i=0; i<1000; i++) {
        sprintf(fnptr, "_%03d.png", i);
        if (!file_exists(filename)) break;
    }
    if (i > 999)
        return NULL;
    return filename;
}

//
//    Screenshot (640x480x32bpp only, rotate180, png)
//
void screenshot(const char *screenshot_path) {
    uint32_t    *buffer;
    uint32_t    *src;
    uint32_t    linebuffer[DISPLAY_WIDTH], x, y, pix;
    FILE        *fp;
    png_structp png_ptr;
    png_infop   info_ptr;

    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    buffer = fb_addr + DISPLAY_WIDTH*vinfo.yoffset;

    mkdirs(dirname(screenshot_path));

    if ((fp = fopen(screenshot_path, "wb"))) {
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, DISPLAY_WIDTH, DISPLAY_HEIGHT, 8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png_ptr, info_ptr);
        src = buffer + DISPLAY_WIDTH * DISPLAY_HEIGHT;
        for (y = 0; y < DISPLAY_HEIGHT; y++) {
            for (x = 0; x < DISPLAY_WIDTH; x++){
                pix = *--src;
                linebuffer[x] = 0xFF000000 | (pix & 0x0000FF00) | (pix & 0x00FF0000)>>16 | (pix & 0x000000FF)<<16;
            }
            png_write_row(png_ptr, (png_bytep)linebuffer);
        }
        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
}

void screenshot_recent(void) {
    char screenshot_name[512];
    if (getrecent_png(screenshot_name) == NULL) return;
    screenshot(screenshot_name);
}

void screenshot_system(void) {
    char screenshot_path[512],
         screenshot_name[256];
    getrecent_onion(screenshot_name);
    sprintf(screenshot_path, "/mnt/SDCARD/Saves/CurrentProfile/romScreens/%s.png", file_removeExtension(screenshot_name));
    screenshot(screenshot_path);
}

#endif // SCREENSHOT_H__