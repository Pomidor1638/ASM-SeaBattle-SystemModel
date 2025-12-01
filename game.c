#include "main.h"
#include <SDL.h>

/*для основной игры*/

const ship_placement_t SHIP_SIZES_AND_COUNT[MAX_SHIPS_SIZE] = {
    {4, 1},  // один 4-палубный
    {3, 2},  // два 3-палубных
    {2, 3},  // три 2-палубных
    {1, 4}   // четыре 1-палубных
};


cur_ship_state_t ship_placement_state;

placement_state_t    my_placement_state;
placement_state_t enemy_placement_state;

int bg_blink_counter;
int warning_counter;

bool wait_remote;
int remote_waiting_counter;
int remote_waiting_attemps;

int server_ready_counter;
int server_ready_attempts;

int cursor_blink_counter;
bool cursor_visible;

int cursor_send_counter;

int16_t    my_cursor_x;
int16_t    my_cursor_y;

int16_t enemy_cursor_x;
int16_t enemy_cursor_y;

bool need_switch;
bool my_turn;
bool all_ships_destroyed;

bool running;
bool server_mode;
uint16_t game_state;

bool CanPlaceShip(cur_ship_state_t* ship_state, placement_state_t* placement_state)
{
    int size = SHIP_SIZES_AND_COUNT[ship_state->ship_size_index].size;
    int ship_x = ship_state->x;
    int ship_y = ship_state->y;
    int check_x, check_y;
    bool horizontal = ship_state->horizontal;

    if (horizontal)
    {
        if (ship_x + size > FIELD_SIZE)
            return false;
    }
    else
    {
        if (ship_y + size > FIELD_SIZE)
            return false;
    }

    for (int i = -1; i <= size; i++)
    {
        for (int j = -1; j <= 1; j++)
        {

            if (horizontal)
            {
                check_x = ship_x + i;
                check_y = ship_y + j;
            }
            else
            {
                check_x = ship_x + j;
                check_y = ship_y + i;
            }

            if (check_x >= 0 && check_x < FIELD_SIZE &&
                check_y >= 0 && check_y < FIELD_SIZE)
            {
                if (placement_state->field[check_y][check_x].state == CELL_SHIP)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

void PutShipToField(cur_ship_state_t* ship_state, placement_state_t* placement_state)
{
    int x, y, i, size;

    size = SHIP_SIZES_AND_COUNT[ship_state->ship_size_index].size;
    x = ship_state->x;
    y = ship_state->y;

    for (i = 0; i < size; i++)
    {
        placement_state->field[y][x].state = CELL_SHIP;
        placement_state->field[y][x].ship_index = placement_state->ships_count;

        if (ship_state->horizontal)
        {
            x++;
        }
        else
        {
            y++;
        }
    }
}

bool PlaceShip(cur_ship_state_t* ship_state, placement_state_t* placement_state)
{
    int ship_size = SHIP_SIZES_AND_COUNT[ship_state->ship_size_index].size;

    int16_t* ships_count = &placement_state->ships_count;
    int16_t* placed_ships = placement_state->placed_ships;
    ship_t*  ships = placement_state->ships;
    ship_t* s;

    if (!CanPlaceShip(ship_state, placement_state))
    {
        return false;
    }


    if (placement_state->ships_count >= MAX_SHIPS)
    {
        return false;
    }

    s = placement_state->ships + *ships_count;
    s->x = ship_state->x;
    s->y = ship_state->y;
    s->size = ship_size;
    s->hits = 0;
    s->horizontal = ship_state->horizontal;

    PutShipToField(ship_state, placement_state);

    (*ships_count)++;
    placed_ships[ship_state->ship_size_index]++;
    if ((*ships_count) >= MAX_SHIPS)
    {
        ship_state->ship_size_index = -1;
        return true;
    }

    if (placed_ships[ship_state->ship_size_index] >= SHIP_SIZES_AND_COUNT[ship_state->ship_size_index].count)
    {
        NextShipSize(ship_state, placement_state);
    }

    return true;
}

void RemoveShip(cur_ship_state_t* ship_state, placement_state_t* placement_state, bool link_cursor)
{
    ship_t* ships = placement_state->ships;
    int16_t* ships_count = &placement_state->ships_count;
    int16_t* placed_ships = placement_state->placed_ships;
    int size_index, x, y, i;

    if (!(*ships_count))
        return;

    (*ships_count)--;
    ship_t* s = ships + (*ships_count);

    size_index = 4 - s->size; 

    if (size_index < 0 || size_index >= MAX_SHIPS_SIZE)
    {
        (*ships_count)++;
        return;
    }

    if (placed_ships[size_index] > 0)
    {
        placed_ships[size_index]--;
    }

    ship_state->ship_size_index = size_index;
    ship_state->horizontal = s->horizontal;
    ship_state->x = s->x;
    ship_state->y = s->y;

    if (link_cursor)
    {
        my_cursor_x = s->x;
        my_cursor_y = s->y;
    }

    x = s->x;
    y = s->y;

    for (i = 0; i < s->size; i++)
    {
        if (x >= 0 && x < FIELD_SIZE && y >= 0 && y < FIELD_SIZE)
        {
            placement_state->field[y][x].state = CELL_EMPTY;
            placement_state->field[y][x].ship_index = -1;
        }

        if (s->horizontal)
        {
            x++;
        }
        else
        {
            y++;
        }
    }
}

void NextShipSize(cur_ship_state_t* ship_state, placement_state_t* placement_state)
{
    int index = ship_state->ship_size_index;
    int16_t* placed_ships = placement_state->placed_ships;

    printf("DEBUG NextShipSize: current_index=%d, placed_ships=[%d,%d,%d,%d]\n",
        index, placed_ships[0], placed_ships[1], placed_ships[2], placed_ships[3]);

    for (int i = 1; i <= MAX_SHIPS_SIZE; i++)
    {
        int next_index = index + i;
        if (next_index >= MAX_SHIPS_SIZE)
        {
            next_index -= MAX_SHIPS_SIZE;
        }

        printf("DEBUG checking index=%d, placed=%d, max=%d\n",
            next_index, placed_ships[next_index], SHIP_SIZES_AND_COUNT[next_index].count);

        if (placed_ships[next_index] < SHIP_SIZES_AND_COUNT[next_index].count)
        {
            ship_state->ship_size_index = next_index;
            printf("DEBUG selected index=%d\n", next_index);
            return;
        }
    }

    outgoing_packet.type = PACKET_REMOTE_ERROR;
    SendPacket();
    game_state = STATE_ERROR;
}

void PrevShipSize(cur_ship_state_t* ship_state, placement_state_t* placement_state)
{
    int index = ship_state->ship_size_index;
    int16_t* placed_ships = placement_state->placed_ships;

    for (int i = 1; i <= MAX_SHIPS_SIZE; i++)
    {
        int prev_index = index - i;
        if (prev_index < 0) 
        {
            prev_index += MAX_SHIPS_SIZE;
        }

        if (placed_ships[prev_index] < SHIP_SIZES_AND_COUNT[prev_index].count)
        {
            ship_state->ship_size_index = prev_index;
            return;
        }
    }

    outgoing_packet.type = PACKET_REMOTE_ERROR;
    SendPacket();
    game_state = STATE_ERROR;
}

static bool DamageShip(ship_t* s, int x, int y)
{
    int h; // hit_pos

    if (s->horizontal)
    {
        h = x - s->x;
    }
    else
    {
        h = y - s->y;
    }

    if (h < 0 || h >= s->size)
    {
        return false;
    }
    
    s->hits |= 1 << h;

    uint8_t all_hits_mask = (1 << s->size) - 1;

    return s->hits == all_hits_mask;
}

static void SunkShip(placement_state_t* placement_state, ship_t* s)
{
    int start_x, end_x, start_y, end_y;

    if (s->horizontal)
    {
        start_x = s->x - 1;
        end_x = s->x + s->size;
        start_y = s->y - 1;
        end_y = s->y + 1;
    }
    else
    {
        start_x = s->x - 1;
        end_x = s->x + 1;
        start_y = s->y - 1;
        end_y = s->y + s->size;
    }

    for (int y = start_y; y <= end_y; y++)
    {
        for (int x = start_x; x <= end_x; x++)
        {
            if (x < 0 || x >= FIELD_SIZE || y < 0 || y >= FIELD_SIZE)
                continue;

            placement_state->field[y][x].state = CELL_MISS;
        }
    }

    if (s->horizontal)
    {
        for (int x = s->x; x < s->x + s->size; x++)
        {
            placement_state->field[s->y][x].state = CELL_SUNK;
        }
    }
    else
    {
        for (int y = s->y; y < s->y + s->size; y++)
        {
            placement_state->field[y][s->x].state = CELL_SUNK;
        }
    }
}

fieldcell_t ShootToField(placement_state_t* placement_state, int x, int y)
{
    fieldcell_t* cell = &placement_state->field[y][x];
    fieldcell_t res = *cell;

    if (x < 0 || x >= FIELD_SIZE || y < 0 || y >= FIELD_SIZE)
        return;

    switch (cell->state)
    {
    case CELL_EMPTY:
        cell->state = CELL_MISS;
        res.state = CELL_MISS;
        break;

    case CELL_SHIP:
    {
        ship_t* s = placement_state->ships + cell->ship_index;



        if (DamageShip(s, x, y))
        {
            SunkShip(placement_state, s);
            res.state = CELL_SUNK;
            bg_blink_counter = 10;
            placement_state->ships_count--;

            if (!placement_state->ships_count)
            {
                all_ships_destroyed = true;
            }

            placement_state->placed_ships[MAX_SHIPS_SIZE - s->size]--;
        }
        else
        {
            cell->state = CELL_HIT;
            res.state = CELL_HIT;
            bg_blink_counter = 5;
        }
    }
    break;

    default:
        res.state = CELL_DONT_SHOT;
        break;
    }

    return res;
}

void ProcessReceivedCell(placement_state_t* placement_state, cellstate_t state, ship_t* s, int x, int y)
{
    fieldcell_t* current_cell = &placement_state->field[y][x];

    if (x < 0 || x >= FIELD_SIZE || y < 0 || y >= FIELD_SIZE)
        return;


    switch (state)
    {
    case CELL_HIT:
        current_cell->state = CELL_HIT;
        bg_blink_counter = 5;
        break;

    case CELL_SUNK:
    {
        SunkShip(placement_state, s);
        bg_blink_counter = 10;
    }
    break;

    case CELL_MISS:
        current_cell->state = CELL_MISS;
        break;

    case CELL_DONT_SHOT:
        break;

    default:
        break;
    }
}

void UpdateCounters()
{
    if (bg_blink_counter)
    {
        bg_blink_counter--;
    }
    if (warning_counter)
    {
        warning_counter--;
    }
    if (cursor_blink_counter)
    {
        cursor_blink_counter--;
    }
    else
    {
        cursor_blink_counter = 5;
        cursor_visible = !cursor_visible;
    }
    if (remote_waiting_counter)
    {
        remote_waiting_counter--;
    }

    if (cursor_send_counter)
        cursor_send_counter--;
    else
        cursor_send_counter = 5;
}

bool UpdateWaitRemote()
{
    if (wait_remote)
    {
        if (!remote_waiting_counter)
        {
            if (!remote_waiting_attemps)
            {
                outgoing_packet.type = PACKET_REMOTE_ERROR;
                SendPacket();
                game_state = STATE_ERROR;
                return true;
            }
            remote_waiting_attemps--;
            remote_waiting_counter = 100;
            SendPacket();
        }
    }
    return false;
}

void StartWaitRemote()
{
    wait_remote = true;
    remote_waiting_attemps = 5;
    remote_waiting_counter = 100;
}

void EndWaitRemote()
{
    wait_remote = false;
    remote_waiting_attemps = 0;
    remote_waiting_counter = 0;
}


bool Game_Init()
{
#ifdef _VIDEO_TEST_
    game_state = STATE_VIDEO_TEST;
#elif defined(_ERROR_TEST_)
    game_state = STATE_ERROR;
#else
    game_state = STATE_CHOOSE_MODE;
#endif

    SDL_memset(&my_placement_state, 0, sizeof(my_placement_state));
    SDL_memset(&enemy_placement_state, 0, sizeof(enemy_placement_state));
    SDL_memset(&ship_placement_state, 0, sizeof(ship_placement_state));
    SDL_memset(&my_placement_state, 0, sizeof(my_placement_state));
    SDL_memset(&enemy_placement_state, 0, sizeof(enemy_placement_state));

    bg_blink_counter = 0;
    warning_counter = 0;

    wait_remote = false;
    remote_waiting_counter = 0;
    remote_waiting_attemps = 0;

    server_ready_counter = 0;
    server_ready_attempts = 0;

    cursor_blink_counter = 0;
    cursor_visible = true;

    my_cursor_x = FIELD_SIZE / 2;
    my_cursor_y = FIELD_SIZE / 2;

    enemy_cursor_x = FIELD_SIZE / 2;
    enemy_cursor_y = FIELD_SIZE / 2;

    running = true;
    server_mode = false;
    need_switch = false;
    my_turn = false;
    all_ships_destroyed = false;;

    return true;
}

void Game_Quit()
{
    running = false;
    game_state = STATE_QUIT;
}
