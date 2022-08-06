#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>  
#include <sys/stat.h>
#include <dirent.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include "utils/log.h"
#include "utils/battery.h"
#include "system/settings.h"
#include "system/keymap_sw.h"
#include "theme/theme.h"
#include "components/menu.h"

#define FRAMES_PER_SECOND 30
#define CHECK_BATTERY_TIMEOUT 30000 //ms

#define RESOURCES { \
	TR_BACKGROUND, \
	TR_BG_TITLE, \
	TR_LOGO, \
	TR_BATTERY_0, \
	TR_BATTERY_20, \
	TR_BATTERY_50, \
	TR_BATTERY_80, \
	TR_BATTERY_100, \
	TR_BATTERY_CHARGING, \
	TR_BG_LIST_S, \
	TR_HORIZONTAL_DIVIDER, \
	TR_TOGGLE_ON, \
	TR_TOGGLE_OFF, \
	TR_BG_FOOTER, \
	TR_BUTTON_A, \
	TR_BUTTON_B \
}
#define NUM_RESOURCES 16

int main(int argc, char *argv[])
{
	print_debug("Debug logging enabled");

	int pargc = 0;
	char **pargs = NULL;
	char title_str[STR_MAX] = "";
	char message_str[STR_MAX] = "";
	bool required = false;

	pargs = malloc(100 * sizeof(char*));

	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--title") == 0) {
				strncpy(title_str, argv[++i], STR_MAX-1);
				continue;
			}
			if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--message") == 0) {
				strncpy(message_str, argv[++i], STR_MAX-1);
				continue;
			}
			if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--required") == 0) {
				required = true;
				continue;
			}
		}
		pargs[pargc] = malloc((STR_MAX + 1) * sizeof(char));
		strncpy(pargs[pargc], argv[i], STR_MAX);
		pargc++;
	}

	if (pargc == 0) {
		pargs[pargc++] = "OK";
		pargs[pargc++] = "CANCEL";
	}

	printf_debug(LOG_SUCCESS, "parsed command line arguments");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableKeyRepeat(300, 50);
	TTF_Init();

	SDL_Surface* video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE);
	SDL_Surface* screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 640, 480, 32, 0, 0, 0, 0);

	// print_debug("Loading settings...");
	// settings_init();
	// printf_debug(LOG_SUCCESS, "loaded settings");

	print_debug("Loading theme config...");
	Theme_s theme = loadThemeFromPath(settings.theme);
	// Theme_s theme = loadThemeFromPath("/mnt/SDCARD/Themes/Blueprint by Aemiii91");
	// Theme_s theme = loadThemeFromPath("/mnt/SDCARD/Themes/Analogue by Aemiii91");
	// Theme_s theme = loadThemeFromPath("/customer/app");
	printf_debug(LOG_SUCCESS, "loaded theme config");

	enum theme_Images res_requests[NUM_RESOURCES] = RESOURCES;
	Resources_s res = theme_loadResources(&theme, res_requests, NUM_RESOURCES);

	print_debug("Reading battery percentage...");
    int current_percentage = battery_getPercentage();
	int old_percentage = current_percentage;
	printf_debug(LOG_SUCCESS, "read battery percentage");
    SDL_Surface* battery = theme_batterySurface(&theme, &res, current_percentage);

	Menu_s menu = menu_create(pargc);
	for (i = 0; i < pargc; i++) {
		menu_addItem(&menu, pargs[i], i);
	}

	bool has_title = strlen(title_str) > 0;

	SDL_Surface *message = NULL;
	SDL_Rect message_rect;
	bool has_message = strlen(message_str) > 0;

	if (has_message) {
		char *str = str_replace(message_str, "\\n", "\n");
		message = theme_textboxSurface(&theme, &res, str, res.fonts.title, theme.grid.color);
		menu.scroll_height = 3;
		message_rect.x = 20;
		message_rect.y = 60 + (6 - menu.scroll_height) * 30 - message->h / 2;
		free(str);
	}

	bool quit = false;
	bool changed = true;
	SDL_Event event;
	Uint8 keystate[320];
	int return_code = -1;

	uint32_t batt_timer = 0,
			 acc_ticks = 0,
			 last_ticks = SDL_GetTicks(),
			 time_step = 1000 / FRAMES_PER_SECOND;

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
				break;
			}

			SDLKey key = event.key.keysym.sym;
			bool repeating = false;
			
			if (event.type == SDL_KEYDOWN) {
				if (keystate[key] == 1)
					repeating = true;
				keystate[key] = 1;
				switch (key) {
					case SW_BTN_UP:
						menu_moveUp(&menu, repeating);
						changed = true;
						break;
					case SW_BTN_DOWN:
						menu_moveDown(&menu, repeating);
						changed = true;
						break;
					default: break;
				}
			}
			else if (event.type == SDL_KEYUP) {
				switch (key) {
					case SW_BTN_A:
						return_code = menu_applyAction(&menu);
						quit = true;
					case SW_BTN_B:
						if (!required)
							quit = true;
						break;
					case SW_BTN_POWER:
					// case SDLK_FIRST:
						quit = true;
						break;
					default: break;
				}
				keystate[key] = 0;
			}
		}

		uint32_t ticks = SDL_GetTicks(),
				 delta = ticks - last_ticks;
		batt_timer += delta;
		acc_ticks += delta;
		last_ticks = ticks;

		if (batt_timer >= CHECK_BATTERY_TIMEOUT) {
			current_percentage = battery_getPercentage();
			batt_timer = 0;

			if (current_percentage != old_percentage) {
				SDL_FreeSurface(battery);
				battery = theme_batterySurface(&theme, &res, current_percentage);
				old_percentage = current_percentage;
				changed = true;
			}
		}

		if (acc_ticks >= time_step) {
			if (changed) {
				theme_renderBackground(&theme, &res, screen);
				theme_renderHeader(&theme, &res, screen, battery, has_title ? title_str : NULL, !has_title);

				if (has_message)
					SDL_BlitSurface(message, NULL, screen, &message_rect);

				theme_renderMenu(&theme, &res, screen, &menu);
				theme_renderListFooter(&theme, &res, screen, &menu, "SELECT", required ? NULL : "BACK");
			
				SDL_BlitSurface(screen, NULL, video, NULL); 
				SDL_Flip(video);

				changed = false;
			}

			acc_ticks -= time_step;
		}
	}
	
	if (has_message)
		SDL_FreeSurface(message);
	SDL_FreeSurface(battery);
   	SDL_FreeSurface(screen);
   	SDL_FreeSurface(video);
    SDL_Quit();

	free(pargs);
	menu_free(&menu);
	theme_freeResources(&res);
	
    return return_code;
}