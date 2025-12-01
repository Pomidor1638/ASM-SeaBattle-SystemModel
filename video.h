#pragma once

#define BUFFER_WIDTH 40
#define BUFFER_HEIGHT 30

extern SDL_Renderer* renderer;

typedef struct {
    uint8_t symbol;
    uint8_t fg_color : 4;
    uint8_t bg_color : 4;
} Cell;

extern Cell video_buffer[BUFFER_HEIGHT][BUFFER_WIDTH];
extern SDL_Color color_palette[16];

bool Video_Init();
void Video_Quit();

void Video_Clear(uint8_t bg_color);
void Video_Render();

void Video_PutChar(int x, int y, char ascii_char, uint8_t fg_color, uint8_t bg_color);
void Video_Print(int x, int y, const char* text, uint8_t fg_color, uint8_t bg_color);
void Video_PrintWrapped(int start_x, int start_y, const char* text, uint8_t fg_color, uint8_t bg_color);
void Video_PrintCentered(int y, const char* text, uint8_t fg_color, uint8_t bg_color);
void Video_Printf(int x, int y, uint8_t fg_color, uint8_t bg_color, bool wrapped, const char* format, ...);

void Video_ChooseMode();
void Video_WaitForConnection();
void Video_PlacingShips();
void Video_DrawShip(int x, int y, int size, bool horizontal);
void Video_Test();
void Video_DrawWaitRemote();


void Video_DrawMyTurn();
void Video_DrawEnemyTurn();