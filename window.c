#include "main.h"


SDL_Window* window = NULL;

bool Window_Init()
{
	window = SDL_CreateWindow("Sea battle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window)
	{
		return false;
	}
	return true;
}

void Window_Quit()
{
	if (window)
		SDL_DestroyWindow(window);
	window = NULL;
}

bool isWindowInFocus() {
	Uint32 flags = SDL_GetWindowFlags(window);
	return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}