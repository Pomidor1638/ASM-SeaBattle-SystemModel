#include "main.h"

Uint8 key_state_last[SDL_NUM_SCANCODES];
Uint8 key_state_cur [SDL_NUM_SCANCODES];

bool Input_Init()
{
	SDL_memset(key_state_last, 0, SDL_NUM_SCANCODES);
	SDL_memset(key_state_cur, 0, SDL_NUM_SCANCODES);
	return true;
}

void Input_Quit()
{
	/*skip, nothing to destroy*/
}

void UpdateLastInputStates()
{
	SDL_memcpy(key_state_last, key_state_cur, SDL_NUM_SCANCODES);
}

void UpdateCurInputStates()
{
	/*nothing to update*/
}

void UpdateKeyboardState(SDL_KeyboardEvent* e)
{
	const SDL_Scancode scancode = e->keysym.scancode;
	if (e->type == SDL_KEYDOWN) {
		key_state_cur[scancode] = 1;
	}
	else if (e->type == SDL_KEYUP) {
		key_state_cur[scancode] = 0;
	}
}

bool isKeyPressed(SDL_Scancode sc)
{
	return key_state_cur[sc];
}

bool isKeyJustPressed(SDL_Scancode sc) { return !key_state_last[sc] && key_state_cur[sc]; }

bool isKeyReleased(SDL_Scancode sc) { return key_state_last[sc] && !key_state_cur[sc]; }

static void FocusIndependentEvent(SDL_Event* e)
{
	switch (e->type)
	{
	case SDL_QUIT:
		running = false;
		return;
	default:
		break;
	}
}

static void FocusDependentEvent(SDL_Event* e)
{
	if (!isWindowInFocus())
	{
		return;
	}

	switch (e->type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		UpdateKeyboardState(&e->key);
		break;
	default:
		break;
	}
}

void Input_Update()
{
	SDL_Event e;
	UpdateLastInputStates();
	while (SDL_PollEvent(&e))
	{
		FocusIndependentEvent(&e);
		FocusDependentEvent(&e);
	}
	UpdateCurInputStates();
}