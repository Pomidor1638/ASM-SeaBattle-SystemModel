#pragma once


bool Input_Init();
void Input_Quit();

void UpdateLastInputStates();
void UpdateCurInputStates();

void UpdateKeyboardState(SDL_KeyboardEvent* e);

bool isKeyPressed(SDL_Scancode sc);
bool isKeyJustPressed(SDL_Scancode sc);
bool isKeyReleased(SDL_Scancode sc);

void Input_Update();
