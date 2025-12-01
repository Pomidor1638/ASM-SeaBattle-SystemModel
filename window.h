#pragma once

#define WIDTH  640
#define HEIGHT 480

extern SDL_Window* window;

bool Window_Init();
void Window_Quit();
bool isWindowInFocus();