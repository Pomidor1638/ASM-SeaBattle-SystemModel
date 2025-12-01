#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>
#include <SDL_net.h>

typedef Uint8 byte;

//#define _VIDEO_TEST_
//#define _ERROR_TEST_

#include "game.h"
#include "window.h"
#include "input.h"
#include "video.h"
#include "net.h"

bool Init();
void Quit();
void Tick();