#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "debug.h"

bool
subystem_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		debug_print("Could not initialize SDL: %s.\n", SDL_GetError());
		return false;
	}

	if (IMG_Init(IMG_INIT_PNG) < 0)
	{
		debug_print("Could not initialize IMG: %s.\n", IMG_GetError());
		return false;
	}

	if (TTF_Init() < 0)
	{
		debug_print("Could not initialize TTF: %s.\n", TTF_GetError());
		return false;
	}

	return true;
}

void
subsystem_close(void)
{
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}