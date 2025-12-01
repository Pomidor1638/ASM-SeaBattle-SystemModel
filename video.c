#include "main.h"
#include "font.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

SDL_Renderer* renderer = NULL;

Cell video_buffer[BUFFER_HEIGHT][BUFFER_WIDTH];

const int char_screen_width = WIDTH / BUFFER_WIDTH;
const int char_screen_height = HEIGHT / BUFFER_HEIGHT;

SDL_Color color_palette[16] = {
    // Яркие основные цвета (0-7)
    {   0,   0,   0, 255}, // 0: черный
    { 255,   0,   0, 255}, // 1: красный
    {   0, 255,   0, 255}, // 2: зеленый
    {   0,   0, 255, 255}, // 3: синий
    { 255, 255,   0, 255}, // 4: желтый
    { 255,   0, 255, 255}, // 5: пурпурный
    {   0, 255, 255, 255}, // 6: голубой
    { 255, 255, 255, 255}, // 7: белый

    // Темные основные цвета (8-15)
    { 128,   0,   0, 255}, // 8: темно-красный
    {   0, 128,   0, 255}, // 9: темно-зеленый
    {   0,   0, 128, 255}, // 10: темно-синий
    { 128, 128,   0, 255}, // 11: темно-желтый
    { 128,   0, 128, 255}, // 12: темно-пурпурный
    {   0, 128, 128, 255}, // 13: темно-голубой
    {  85,  85,  85, 255}, // 14: темно-серый
    { 128, 128, 128, 255}  // 15: серый
};

#define WATER_FRONT_COLOR     0
#define WATER_SYMBOL          0
#define WATER_BACK_COLOR     10

#define FIELD_WALL_COLOR     14
#define CHAR_OVER_WALL_COLOR  7

#define SHIP_SYMBOL ('#' - 32)
#define SHIP_FRONT_COLOR     15
#define SHIP_PLACING_FRONT_COLOR     1
#define SHIP_BACK_COLOR      10 

#define MISS_FRONT_COLOR     15
#define MISS_BACK_COLOR      10

#define HIT_SYMBOL ('X' - 32)
#define HIT_FRONT_COLOR       1  
#define HIT_BACK_COLOR       10  

#define SUNK_SYMBOL DITHER_LEVEL3

#define SUNK_FRONT_COLOR      8  
#define SUNK_BACK_COLOR      10  

#define CURSOR_SYMBOL         ('+' - 32) 
#define CURSOR_FRONT_COLOR    4  
#define CURSOR_BACK_COLOR     SHIP_BACK_COLOR 

#define PLAYER_SHIP_COLOR     2 
#define ENEMY_SHIP_COLOR     15 

#define BORDER_FRONT_COLOR    7 
#define BORDER_BACK_COLOR    14 

#define WATER_FRONT_COLOR     0
#define WATER_SYMBOL          0
#define WATER_BACK_COLOR     10
#define FIELD_WALL_COLOR     14
#define CHAR_OVER_WALL_COLOR  1

#define WARNING_TEXT_FG_COLOR 1
#define WARNING_TEXT_BG_COLOR 0

SDL_Texture* font_atlas = NULL;

bool Video_CreateFontAtlas() {
    int atlas_width = FONT_WIDTH * ATLAS_COLS;
    int atlas_height = FONT_HEIGHT * ATLAS_ROWS;

    SDL_Surface* atlas_surface = SDL_CreateRGBSurfaceWithFormat(0,
        atlas_width, atlas_height, 32, SDL_PIXELFORMAT_RGBA32);

    if (!atlas_surface)
        return false;

    SDL_FillRect(atlas_surface, NULL, SDL_MapRGBA(atlas_surface->format, 0, 0, 0, 0));

    for (int symbol = 0; symbol < NUM_SYMBOLS; symbol++) {
        int atlas_x = (symbol % ATLAS_COLS) * FONT_WIDTH;
        int atlas_y = (symbol / ATLAS_COLS) * FONT_HEIGHT;

        uint8_t* char_data = font8x8[symbol];
        for (int y = 0; y < FONT_HEIGHT; y++) {
            uint8_t row = char_data[y];
            for (int x = 0; x < FONT_WIDTH; x++) {
                bool pixel_on = (row >> x) & 1;
                if (pixel_on) {
                    Uint32* pixels = (Uint32*)atlas_surface->pixels;
                    int pixel_index = (atlas_y + y) * atlas_width + (atlas_x + x);
                    pixels[pixel_index] = SDL_MapRGBA(atlas_surface->format, 255, 255, 255, 255);
                }
            }
        }
    }
    font_atlas = SDL_CreateTextureFromSurface(renderer, atlas_surface);
    SDL_FreeSurface(atlas_surface);

    if (!font_atlas)
        return false;

    SDL_SetTextureBlendMode(font_atlas, SDL_BLENDMODE_BLEND);
    return true;
}

bool Video_Init()
{
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        return false;
    }

    for (int y = 0; y < BUFFER_HEIGHT; y++) {
        for (int x = 0; x < BUFFER_WIDTH; x++) {
            video_buffer[y][x].symbol = 0;
            video_buffer[y][x].fg_color = 7;
            video_buffer[y][x].bg_color = 0;
        }
    }

    return Video_CreateFontAtlas();
}

void Video_Quit()
{
    if (font_atlas) {
        SDL_DestroyTexture(font_atlas);
        font_atlas = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
}

void Video_Render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int buffer_y = 0; buffer_y < BUFFER_HEIGHT; buffer_y++)
    {
        for (int buffer_x = 0; buffer_x < BUFFER_WIDTH; buffer_x++)
        {
            Cell cell = video_buffer[buffer_y][buffer_x];

            const SDL_Rect dest_rect = {
                buffer_x * char_screen_width,
                buffer_y * char_screen_height,
                char_screen_width,
                char_screen_height
            };

            SDL_Color bg_color = color_palette[cell.bg_color];
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &dest_rect);

            if (cell.symbol != 0 && cell.symbol < NUM_SYMBOLS)
            {
                const SDL_Rect src_rect =
                {
                    .x = (cell.symbol % ATLAS_COLS) * FONT_WIDTH,
                    .y = (cell.symbol / ATLAS_COLS) * FONT_HEIGHT,
                    .w = FONT_WIDTH,
                    .h = FONT_HEIGHT
                };

                SDL_Color fg_color = color_palette[cell.fg_color];
                SDL_SetTextureColorMod(font_atlas, fg_color.r, fg_color.g, fg_color.b);
                SDL_RenderCopy(renderer, font_atlas, &src_rect, &dest_rect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void Video_Clear(uint8_t bg_color)
{
    for (int y = 0; y < BUFFER_HEIGHT; y++)
    {
        for (int x = 0; x < BUFFER_WIDTH; x++)
        {
            video_buffer[y][x].symbol = 0;
            video_buffer[y][x].fg_color = 7;
            video_buffer[y][x].bg_color = bg_color;
        }
    }
}

void Video_PutSymbol(int x, int y, uint8_t symbol, uint8_t fg_color, uint8_t bg_color)
{
    if (x >= 0 && x < BUFFER_WIDTH && y >= 0 && y < BUFFER_HEIGHT)
    {
        if (symbol >= NUM_SYMBOLS) symbol = 0;
        video_buffer[y][x].symbol = symbol;
        video_buffer[y][x].fg_color = fg_color;
        video_buffer[y][x].bg_color = bg_color;
    }
}

void Video_PutChar(int x, int y, char ascii_char, uint8_t fg_color, uint8_t bg_color) {

    if (x >= 0 && x < BUFFER_WIDTH && y >= 0 && y < BUFFER_HEIGHT)
    {
        uint8_t symbol;

        if (ascii_char >= 32 && ascii_char <= 127)
        {
            symbol = ascii_char - 32;
        }
        else
        {
            symbol = 0;
        }

        if (symbol >= NUM_SYMBOLS) symbol = 0;
        video_buffer[y][x].symbol = symbol;
        video_buffer[y][x].fg_color = fg_color;
        video_buffer[y][x].bg_color = bg_color;
    }
}

void Video_Print(int x, int y, const char* text, uint8_t fg_color, uint8_t bg_color)
{
    for (int i = 0; text[i] != '\0'; i++)
    {
        Video_PutChar(x + i, y, text[i], fg_color, bg_color);
    }
}

void Video_PrintWrapped(int start_x, int start_y, const char* text, uint8_t fg_color, uint8_t bg_color)
{
    int x = start_x;
    int y = start_y;

    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == '\n')
        {
            x = start_x;
            y++;
            continue;
        }

        if (x >= BUFFER_WIDTH)
        {
            x = start_x;
            y++;
        }

        if (y >= BUFFER_HEIGHT)
        {
            break;
        }

        Video_PutChar(x, y, text[i], fg_color, bg_color);
        x++;
    }
}

void Video_Printf(int x, int y, uint8_t fg_color, uint8_t bg_color, bool wrapped, const char* format, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (wrapped)
    {
        Video_PrintWrapped(x, y, buffer, fg_color, bg_color);
    }
    else
    {
        Video_Print(x, y, buffer, fg_color, bg_color);
    }
}

void Video_PrintCentered(int y, const char* text, uint8_t fg_color, uint8_t bg_color)
{

    int len = 0;
    while (text[len] != '\0') len++;

    int x = (BUFFER_WIDTH - len) / 2;
    if (x < 0) x = 0;

    Video_Print(x, y, text, fg_color, bg_color);
}

void Video_ChooseMode()
{
    Video_Clear(0);

    Video_PrintCentered(3, " SEA BATTLE ", 2, 14);
    Video_PrintCentered(5, "CHOOSE DEVICE MODE", 2, 0);

    int client_x = BUFFER_WIDTH / 4;
    int server_x = 3 * BUFFER_WIDTH / 4;
    int option_y = BUFFER_HEIGHT / 2;

    if (server_mode)
    {
        Video_PrintCentered(option_y - 2, " client", 2, 0);
        Video_PrintCentered(option_y + 2, ">SERVER", 0, 2);
    }
    else
    {
        Video_PrintCentered(option_y - 2, ">CLIENT", 0, 2);
        Video_PrintCentered(option_y + 2, " server", 2, 0);
    }

    Video_PrintCentered(BUFFER_HEIGHT - 3, "Press UP/DOWN to change", 14, 0);
    Video_PrintCentered(BUFFER_HEIGHT - 2, "Press ENTER to confirm", 14, 0);
}

void Video_WaitForConnection()
{
    static int dot_counter = 0;
    static Uint32 last_time = 0;

    Uint32 current_time = SDL_GetTicks();

    if (current_time - last_time > 500) {
        dot_counter = (dot_counter + 1) % 4;
        last_time = current_time;
    }

    Video_Clear(0);

    Video_PrintCentered(10, "WAITING FOR CONNECTION", 7, 0);

    if (server_mode)
        Video_PrintCentered(12, "Waiting for client", 7, 0);
    else
        Video_PrintCentered(12, "Waiting for server", 7, 0);


    char dots[5] = "   ";
    for (int i = 0; i < dot_counter; i++)
    {
        dots[i] = '.';
    }

    Video_PrintCentered(14, dots, 7, 0);
    Video_PrintCentered(BUFFER_HEIGHT - 3, "Press ESC to cancel", 8, 0);
}



static void Video_DrawField(int start_x, int start_y, fieldcell_t field[FIELD_SIZE][FIELD_SIZE], bool draw_ships)
{
    if (!field)
        return;

    const int grid_size = FIELD_SIZE + 2;
    const int end_x = start_x + grid_size - 1;
    const int end_y = start_y + grid_size - 1;

    int symbol;
    int front_color;
    int back_color = WATER_BACK_COLOR;

    int x, y;

    for (int i = 0; i < FIELD_SIZE; i++)
    {
        for (int j = 0; j < FIELD_SIZE; j++)
        {
            x = j + start_x + 1;
            y = i + start_y + 1;

            fieldcell_t* current_cell = &(field[i][j]);

            symbol = WATER_SYMBOL;
            front_color = WATER_FRONT_COLOR;
            back_color = WATER_BACK_COLOR;

            switch (current_cell->state)
            {
            case CELL_EMPTY:
                symbol = WATER_SYMBOL;
                front_color = WATER_FRONT_COLOR;
                break;
            case CELL_SHIP:
                if (draw_ships)
                {
                    symbol = SHIP_SYMBOL;
                    front_color = SHIP_FRONT_COLOR;
                }
                break;
            case CELL_MISS:
                symbol = DITHER_LEVEL1;
                front_color = MISS_FRONT_COLOR;
                break;
            case CELL_HIT:
                symbol = HIT_SYMBOL;
                front_color = HIT_FRONT_COLOR;
                break;
            case CELL_SUNK:
                symbol = SUNK_SYMBOL;
                front_color = HIT_FRONT_COLOR;
                break;
            default:
                symbol = WATER_SYMBOL;
                front_color = WATER_FRONT_COLOR;
                break;
            }
            Video_PutSymbol(x, y, symbol, front_color, back_color);
        }
    }

    for (int i = 0; i < FIELD_SIZE; i++)
    {

        char digit = '0' + i;
        char charecter = 'A' + i;

        Video_PutChar(start_x + i + 1, start_y, charecter, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
        Video_PutChar(start_x + i + 1, end_y, charecter, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);


        Video_PutChar(start_x, start_y + i + 1, digit, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
        Video_PutChar(end_x, start_y + i + 1, digit, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
    }

    Video_PutChar(start_x, start_y, 0, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
    Video_PutChar(start_x, end_y, 0, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
    Video_PutChar(end_x, end_y, 0, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
    Video_PutChar(end_x, start_y, 0, CHAR_OVER_WALL_COLOR, FIELD_WALL_COLOR);
}

static void Video_DrawFields(int cursor_x, int cursor_y, bool my_turn)
{
    const int field_size = FIELD_SIZE + 2;
    const int spacing = 4;

    int total_width = 2 * field_size + spacing;
    int my_field_start_x = (BUFFER_WIDTH - total_width) / 2;
    int my_field_start_y = (BUFFER_HEIGHT - field_size) / 2;

    int enemy_field_start_x = my_field_start_x + field_size + spacing;
    int enemy_field_start_y = my_field_start_y;
    int x, y;

    if (my_turn)
    {
        x = cursor_x + enemy_field_start_x + 1;
        y = cursor_y + enemy_field_start_y + 1;
    }
    else
    {
        x = cursor_x + my_field_start_x + 1;
        y = cursor_y + my_field_start_y + 1;
    }

    Video_DrawField(my_field_start_x, my_field_start_y, my_placement_state.field, true);
    Video_DrawField(enemy_field_start_x, enemy_field_start_y, enemy_placement_state.field, false);

    if (cursor_visible)
        Video_PutSymbol(x, y, CURSOR_SYMBOL, my_turn ? 2 : 1, CURSOR_BACK_COLOR);
}


void Video_DrawShip(int x, int y, int size, bool horizontal)
{
    for (int i = 0; i < size; i++)
    {
        Video_PutSymbol(x, y, SHIP_SYMBOL, SHIP_PLACING_FRONT_COLOR, SHIP_BACK_COLOR);
        if (horizontal)
            x++;
        else
            y++;
    }
}

void Video_PlacingShips()
{

    int i, j, x, y;

    const int grid_size = FIELD_SIZE + 2;
    const int start_x = (BUFFER_WIDTH - grid_size) / 2;
    const int start_y = (BUFFER_HEIGHT - grid_size) / 2;

    Video_DrawField(start_x, start_y, my_placement_state.field, true);

    if (my_placement_state.ships_count < MAX_SHIPS && cursor_visible)
        Video_DrawShip(start_x + 1 + my_cursor_x, start_y + 1 + my_cursor_y, SHIP_SIZES_AND_COUNT[ship_placement_state.ship_size_index].size, ship_placement_state.horizontal);

    if (warning_counter)
    {
        Video_PrintCentered(BUFFER_HEIGHT - 2, "BAD SHIP PLACEMENT", 1, 0);
    }

    if (enemy_placement_state.ready)
    {
        Video_PrintCentered(6, "Enemy is READY", 1, 0);
    }

    if (my_placement_state.ready)
    {
        Video_PrintCentered(BUFFER_HEIGHT - 4, "READY", 7, 0);
    }
    else if (my_placement_state.ships_count == MAX_SHIPS)
    {
        Video_PrintCentered(BUFFER_HEIGHT - 4, "Are you READY?", 7, 0);
    }

    for (i = 0; i < MAX_SHIPS_SIZE; i++)
    {
        y = BUFFER_HEIGHT / 2 - 4 + (i << 1);

        x = 3;

        Video_PutChar(x, y, '0' + my_placement_state.placed_ships[i], 7, 0);
        Video_PutChar(x + 1, y, '/', 7, 0);
        Video_PutChar(x + 2, y, '0' + SHIP_SIZES_AND_COUNT[i].count, 7, 0);

        for (j = 0; j < SHIP_SIZES_AND_COUNT[i].size; j++)
        {
            Video_PutSymbol(x + 4 + j, y, SHIP_SYMBOL, SHIP_FRONT_COLOR, 0);
        }

    }

}
void Video_Error()
{
    Video_Clear(1);

    Video_PrintCentered(BUFFER_HEIGHT / 2 - 1, "UNEXCEPTED ERROR", 0, 1);
    Video_PrintCentered(BUFFER_HEIGHT / 2 + 1, "PROCESSOR IS HALTED", 0, 1);
}

void Video_DrawWaitRemote()
{
    if (server_mode)
        Video_Print(1, 1, "Waiting for Server", 7, 0);
    else
        Video_Print(1, 1, "Waiting for Client", 7, 0);
}

void Video_Test()
{
    Video_Clear(0);

    int cols = 16;
    int rows = (NUM_SYMBOLS + cols - 1) / cols;

    int table_width = cols + 3;
    int table_height = rows + 2;

    int start_x = (BUFFER_WIDTH - table_width) / 2;
    int start_y = (BUFFER_HEIGHT - table_height) / 2;

    int total_combos = 16 * 15;
    for (int y = 0; y < rows; y++) {
        if (y < 10) {
            Video_Printf(start_x, start_y + 1 + y, 15, 0, false, "%d", y);
        }
        else {
            Video_Printf(start_x, start_y + 1 + y, 15, 0, false, "%c", 'A' + (y - 10));
        }
    }

    for (int x = 0; x < cols; x++) {
        if (x < 10) {
            Video_Printf(start_x + 2 + x, start_y, 15, 0, false, "%d", x);
        }
        else {
            Video_Printf(start_x + 2 + x, start_y, 15, 0, false, "%c", 'A' + (x - 10));
        }
    }
    Video_Printf(start_x, start_y, 15, 0, false, " ");

    for (int i = 0; i < NUM_SYMBOLS; i++) {
        int x = i % cols;
        int y = i / cols;

        Video_PutSymbol(start_x + 2 + x, start_y + 1 + y, i, 7, 0);
    }

    Video_PrintCentered(start_y - 2, "SYMBOL TABLE", 15, 0);
}


void Video_DrawMyTurn()
{
    Video_DrawFields(my_cursor_x, my_cursor_y, true);
    Video_Print(BUFFER_WIDTH / 2 - 1, BUFFER_HEIGHT / 2, "->", 7, 0);
    Video_PrintCentered(6, "YOUR TURN", 7, 0);
}

void Video_DrawEnemyTurn()
{
    Video_DrawFields(enemy_cursor_x, enemy_cursor_y, false);
    Video_Print(BUFFER_WIDTH / 2 - 1, BUFFER_HEIGHT / 2, "<-", 7, 0);
    Video_PrintCentered(6, "ENEMY'S TURN", 1, 0);
}