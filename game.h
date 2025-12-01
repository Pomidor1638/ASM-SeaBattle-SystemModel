#pragma once

#include <SDL.h>
#include <stdbool.h>

#define FIELD_SIZE 10
#define MAX_SHIPS  10
#define MAX_SHIPS_SIZE 4

typedef enum
{
    CELL_EMPTY = 0,
    CELL_SHIP,
    CELL_MISS,
    CELL_HIT,
    CELL_SUNK,
    CELL_DONT_SHOT
} cellstate_t;

typedef struct
{
    uint8_t state;
    uint8_t ship_index;
} fieldcell_t;

typedef struct
{
    uint8_t          x : 4;
    uint8_t          y : 4;
    uint8_t       size : 3;
    uint8_t       hits : 4;
    uint8_t horizontal : 1;
} ship_t;

typedef struct
{

    int16_t placed_ships[MAX_SHIPS_SIZE];
    fieldcell_t field[FIELD_SIZE][FIELD_SIZE];

    int16_t ships_count;
    ship_t  ships[MAX_SHIPS];

    bool ready;

} placement_state_t;

typedef struct
{
    int16_t size;
    int16_t count;
} ship_placement_t;

extern const ship_placement_t SHIP_SIZES_AND_COUNT[MAX_SHIPS_SIZE];

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t ship_size_index;
    bool horizontal;
} cur_ship_state_t;

extern cur_ship_state_t ship_placement_state;

extern placement_state_t    my_placement_state;
extern placement_state_t enemy_placement_state;

// feedback
extern int bg_blink_counter;
extern int warning_counter;

// client only
extern bool wait_remote;
extern int remote_waiting_counter;
extern int remote_waiting_attemps;

// for game beginning
extern int server_ready_counter;
extern int server_ready_attempts;

// for cursor blinking
extern int cursor_blink_counter;
extern bool cursor_visible;

// Please don't crash my router
extern int cursor_send_counter;

extern int16_t    my_cursor_x;
extern int16_t    my_cursor_y;

extern int16_t enemy_cursor_x;
extern int16_t enemy_cursor_y;

extern bool need_switch;
extern bool my_turn;
extern bool all_ships_destroyed;

// Для всей программы
enum GAME_STATE
{
    STATE_INIT = 0,
    STATE_CHOOSE_MODE,
    STATE_WAIT_FOR_CONNECTION,
    STATE_PLACING_SHIPS,
    STATE_WAITING_REMOTE_READY,
    STATE_ERROR,
    STATE_MAIN_GAME,
    STATE_GAME_OVER,
    STATE_VIDEO_TEST,
    STATE_GAME_END,
    STATE_QUIT,
};

extern bool running;
extern bool server_mode;
extern uint16_t game_state;

bool PlaceShip(cur_ship_state_t* ship_state, placement_state_t* placement_state);
void NextShipSize(cur_ship_state_t* ship_state, placement_state_t* placement_state);
void RemoveShip(cur_ship_state_t* ship_state, placement_state_t* placement_state, bool link_cursor);
void PrevShipSize(cur_ship_state_t* ship_state, placement_state_t* placement_state);

fieldcell_t ShootToField(placement_state_t* placement_state, int x, int y);
void ProcessReceivedCell(placement_state_t* placement_state, cellstate_t state, ship_t* s, int x, int y);

void UpdateCounters();

bool UpdateWaitRemote();
void StartWaitRemote();
void EndWaitRemote();

bool Game_Init();
void Game_Quit();