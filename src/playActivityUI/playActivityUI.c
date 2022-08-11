#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sys/ioctl.h"

#include "system/system.h"
#include "system/keymap_hw.h"
#include "utils/utils.h"

// Max number of records in the DB
#define MAXVALUES 1000
#define PLAY_ACTIVITY_DB_PATH "/mnt/SDCARD/Saves/CurrentProfile/saves/playActivity.db"

typedef struct structRom
{
    char name[100];
    int playTime;
} rom_list_s;
static rom_list_s rom_list[MAXVALUES];
static int rom_list_len = 0;

int readRomDB()
{
    FILE *fp;

      // Check to avoid corruption
      if (file_exists(PLAY_ACTIVITY_DB_PATH)) {
        if ((fp = fopen(PLAY_ACTIVITY_DB_PATH, "rb")) != NULL) {
            fread(rom_list, sizeof(rom_list), 1, fp);
             rom_list_len = 0;

            for (int i = 0; i < MAXVALUES; i++){
                if (strlen(rom_list[i].name) != 0)
                    rom_list_len++;
            }

            fclose(fp);
        }
        else {
            // The file exists but could not be opened
            // Something went wrong, the program is terminated
            return -1;
        }
    }

    return 1;
}

void writeRomDB(void)
{
    FILE *fp;

    if ((fp = fopen(PLAY_ACTIVITY_DB_PATH, "wb")) != NULL) {
        fwrite(rom_list, sizeof(rom_list), 1, fp);
        fclose(fp);
    }
}

void displayRomDB(void)
{
    printf("--------------------------------\n");
    for (int i = 0 ; i < rom_list_len ; i++) {
            printf("rom_list name: %s\n", rom_list[i].name);

            char cPlayTime[15];
            sprintf(cPlayTime, "%d", rom_list[i].playTime);
            printf("playtime: %s\n", cPlayTime);
     }
    printf("--------------------------------\n");

}

int searchRomDB(char* romName)
{
    int position = -1;

    for (int i = 0; i < rom_list_len; i++) {
        if (strcmp(rom_list[i].name, romName) == 0){
            position = i;
            break;
        }
    }

    return position;
}

int main(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_ShowCursor(SDL_DISABLE);
    TTF_Init();
    int input_fd;
    struct input_event ev;

    // Prepare for Poll HW_BTN input
    input_fd = open("/dev/input/event0", O_RDONLY);

    SDL_Surface* video = SDL_SetVideoMode(640,480, 32, SDL_HWSURFACE);
    SDL_Surface* screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 640,480, 32, 0,0,0,0);

    TTF_Font* font40 = TTF_OpenFont("/customer/app/Exo-2-Bold-Italic.ttf", 40);
    TTF_Font* font25 = TTF_OpenFont("/customer/app/Exo-2-Bold-Italic.ttf", 25);
    TTF_Font* font30 = TTF_OpenFont("/customer/app/Exo-2-Bold-Italic.ttf", 30);
    TTF_Font* fontRomName25 = TTF_OpenFont("/customer/app/wqy-microhei.ttc", 25);

    SDL_Color color_white = {255, 255, 255};
    SDL_Color color_lilla = {136, 97, 252};

    SDL_Surface* imageBackground = IMG_Load("background.png");

    SDL_Surface* imageRomPosition;
    SDL_Surface* imageRomPlayTime;
    SDL_Surface* imageRomName;

    SDL_Surface* imagePages;
    SDL_Surface* imageMileage;

    // Loading DB
    if (readRomDB() == -1)
        // To avoid a DB overwrite
        return EXIT_SUCCESS;

    //displayRomDB();
    // Sorting DB
    rom_list_s tempStruct;
    int bFound = 1;

    while (bFound == 1){
        bFound = 0;
        for (int i = 0 ; i < (rom_list_len-1) ; i++){
            if (rom_list[i].playTime < rom_list[i+1].playTime){
                tempStruct = rom_list[i+1];
                rom_list[i+1] = rom_list[i];
                rom_list[i] = tempStruct;
                bFound = 1 ;
            }
        }
    }

    //writeRomDB();

    //    Mileage
    int ntotalTime = 0;
    for (int i = 0; i < rom_list_len; i++)
        ntotalTime += rom_list[i].playTime;

    char cTotalHandheldMileage[30];

    //sprintf(cPages, "%d",ntotalTime);
    //logMessage("Mileage");
    //logMessage(cPages);
    int h, m;
    h = ntotalTime / 3600;
    m = (ntotalTime - 3600 * h) / 60;
    sprintf(cTotalHandheldMileage, "%d:%02d", h, m);

    //displayRomDB();

    int nPages = (int)((rom_list_len - 1) / 4 + 1);
    char cMessage[50];

    SDL_BlitSurface(imageBackground, NULL, screen, NULL);

    SDL_Rect rectPosition;
    SDL_Rect rectRomPlayTime;
    SDL_Rect rectRomNames;

    SDL_Rect rectPages = { 561, 430, 90, 44};
    SDL_Rect rectMileage = { 484, 8, 170, 42};

    int nCurrentPage = 0;
    char cPosition[5];
    char cPlayTime[20];
    char cTotalTimePlayed[50];

    char cPages[10];

    sprintf(cPages, "%d/%d", nCurrentPage + 1, nPages);
    imagePages = TTF_RenderUTF8_Blended(font30, cPages, color_white);
    imageMileage = TTF_RenderUTF8_Blended(font30, cTotalHandheldMileage, color_white);

    for (int i = 0 ; i < 4 ; i++) {
        sprintf(cPosition, "%d", i+1);
        h = rom_list[i].playTime / 3600;
        m = (rom_list[i].playTime - 3600 * h) / 60;

        if (strlen(rom_list[i].name) != 0)
            sprintf(cTotalTimePlayed, "%d:%02d", h, m);
        else
            sprintf(cTotalTimePlayed, "");

        char *bnameWOExt = file_removeExtension(rom_list[i].name);
        imageRomPosition = TTF_RenderUTF8_Blended(font40, cPosition, color_lilla);
        imageRomPlayTime = TTF_RenderUTF8_Blended(font40, cTotalTimePlayed, color_white);
        imageRomName = TTF_RenderUTF8_Blended(fontRomName25, bnameWOExt , color_white);

        SDL_Rect rectPosition = { 16, 78 + 90 * i, 76, 39};
        SDL_Rect rectRomPlayTime = { 77, 66 + 90 * i, 254, 56};
        SDL_Rect rectRomNames = { 78, 104 + 90 * i, 600, 40};

        SDL_BlitSurface(imageRomPosition, NULL, screen, &rectPosition);
        SDL_BlitSurface(imageRomPlayTime, NULL, screen, &rectRomPlayTime);
        SDL_BlitSurface(imageRomName, NULL, screen, &rectRomNames);
    }

    SDL_BlitSurface(imagePages, NULL, screen, &rectPages);
    SDL_BlitSurface(imageMileage, NULL, screen, &rectMileage);

    SDL_BlitSurface(screen, NULL, video, NULL);
    SDL_Flip(video);

    signed int keystate[120] = {0};

    while(read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_KEY || ev.value > 1) continue;

        keystate[ev.code] = ev.value;

        if (keystate[HW_BTN_POWER]) {
            system_shutdown();
            break;
        }

        if (keystate[HW_BTN_B])
            break;

        if (keystate[HW_BTN_RIGHT]) {
            if (nCurrentPage < nPages - 1)
                nCurrentPage ++;
        }

        if (keystate[HW_BTN_LEFT]) {
            if (nCurrentPage > 0)
                nCurrentPage --;
        }

        // Page update
        SDL_BlitSurface(imageBackground, NULL, screen, NULL);

        sprintf(cPages, "%d/%d", nCurrentPage + 1, nPages);
        imagePages = TTF_RenderUTF8_Blended(font30, cPages, color_white);

        SDL_BlitSurface(imagePages, NULL, screen, &rectPages);
        SDL_BlitSurface(imageMileage, NULL, screen, &rectMileage);

        for (int i = 0; i < 4; i++) {
			rom_list_s curr = rom_list[nCurrentPage * 4 + i];
            sprintf(cPosition, "%d", (int)(nCurrentPage * 4 + i + 1));
            h = curr.playTime / 3600;
            m = (curr.playTime - 3600 * h) / 60;

            if (strlen(curr.name) != 0)
                sprintf(cTotalTimePlayed, "%d:%02d", h,m);
            else
                sprintf(cTotalTimePlayed, "");

            char *bnameWOExt = file_removeExtension(curr.name);
            imageRomPosition = TTF_RenderUTF8_Blended(font40, cPosition, color_lilla);
            imageRomPlayTime = TTF_RenderUTF8_Blended(font40, cTotalTimePlayed, color_white);
            imageRomName = TTF_RenderUTF8_Blended(fontRomName25, bnameWOExt , color_white);

            SDL_Rect rectPosition = { 16, 78 + 90 * i, 76, 39};
            SDL_Rect rectRomPlayTime = { 77, 66 + 90 * i, 254, 56};
            SDL_Rect rectRomNames = { 78, 104 + 90 * i, 600, 40};

            SDL_BlitSurface(imageRomPosition, NULL, screen, &rectPosition);
            SDL_BlitSurface(imageRomPlayTime, NULL, screen, &rectRomPlayTime);
            SDL_BlitSurface(imageRomName, NULL, screen, &rectRomNames);
        }

        SDL_BlitSurface(screen, NULL, video, NULL);
        SDL_Flip(video);
    }

    SDL_FreeSurface(screen);
    SDL_FreeSurface(video);
    SDL_Quit();

    return EXIT_SUCCESS;
}