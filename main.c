#include "main.h"



static bool API_Init()
{
	return !SDL_Init(SDL_INIT_EVERYTHING);
}

static bool API_Quit()
{
	SDL_Quit();
}

bool Init()
{
	running = false;
	server_mode = false;
	typedef bool (*func_t)(void);
	func_t funcs[] =
	{
		API_Init,
		Window_Init,
		Input_Init,
		Net_Init,
		Video_Init,
		Game_Init,
	};

	const int num_funcs = sizeof(funcs) / sizeof(funcs[0]);

	for (int i = 0; i < num_funcs; i++)
	{
		if (!(funcs[i])())
		{
			return false;
		}
	}

	return true;
}

void Quit()
{
	typedef void (*func_t)(void);
	func_t funcs[] =
	{
		Game_Quit,
		Video_Quit,
		Net_Quit,
		Input_Quit,
		Window_Quit,
		API_Quit,
	};

	const int num_funcs = sizeof(funcs) / sizeof(funcs[0]);

	for (int i = 0; i < num_funcs; i++)
	{
		funcs[i]();
	}
}


void TickChooseMode()
{

	if (isKeyJustPressed(SDL_SCANCODE_UP))
	{
		server_mode = false;
	}
	else
		if (isKeyJustPressed(SDL_SCANCODE_DOWN))
		{
			server_mode = true;
		}

	if (isKeyJustPressed(SDL_SCANCODE_RETURN))
	{
		game_state = STATE_WAIT_FOR_CONNECTION;
		Net_SetMode(server_mode);
	}

	Video_ChooseMode();
}

void TickWaitForConnection()
{

	if (isKeyJustPressed(SDL_SCANCODE_ESCAPE))
	{
		game_state = STATE_CHOOSE_MODE;
		Net_CloseSocket();
		return;
	}
	if (isKeyJustPressed(SDL_SCANCODE_SPACE))
	{
		outgoing_packet.type = server_mode ? PACKET_CONNECTION_ACCEPTED : PACKET_CONNECTION_REQUEST;
		SendPacket();
	}

	while (Check_Packet())
	{
		Pop_Packet();
		printf("RECEIVED: Packet type=%d, hash=%d\n", incomming_packet.type, incomming_packet.hash);

		if (server_mode)
		{
			if (incomming_packet.type == PACKET_CONNECTION_REQUEST)
			{
				printf("SUCCESS: Server got request! Sending response...\n");
				outgoing_packet.type = PACKET_CONNECTION_ACCEPTED;
				SendPacket();
				game_state = STATE_PLACING_SHIPS;
			}
		}
		else
		{
			if (incomming_packet.type == PACKET_CONNECTION_ACCEPTED)
			{
				printf("SUCCESS: Client got response! Connecting...\n");
				game_state = STATE_PLACING_SHIPS;
			}
		}
	}
	if (!server_mode)
	{
		static Uint32 last_request_time = 0;
		Uint32 current_time = SDL_GetTicks();

		if (current_time - last_request_time > 1000)
		{
			printf("CLIENT: Auto-sending request...\n");
			outgoing_packet.type = PACKET_CONNECTION_REQUEST;
			SendPacket();
			last_request_time = current_time;
		}
	}


	Video_WaitForConnection();
}


void HandleCursorMovement()
{

	if (isKeyJustPressed(SDL_SCANCODE_UP))
	{
		if (my_cursor_y)
			my_cursor_y -= 1;
	}
	if (isKeyJustPressed(SDL_SCANCODE_DOWN))
	{
		if (my_cursor_y + 1 < FIELD_SIZE)
			my_cursor_y += 1;
	}
	if (isKeyJustPressed(SDL_SCANCODE_LEFT))
	{
		if (my_cursor_x)
			my_cursor_x -= 1;
	}
	if (isKeyJustPressed(SDL_SCANCODE_RIGHT))
	{
		if (my_cursor_x + 1 < FIELD_SIZE)
			my_cursor_x += 1;
	}
}

void ServerHandleClientPlacement()
{
	cur_ship_state_t cl_ship_state;

	while (Check_Packet())
	{
		Pop_Packet();

		switch (incomming_packet.type)
		{
		case PACKET_SHIP_PLACE_REQUEST:

			cl_ship_state.x = incomming_packet.data[0];
			cl_ship_state.y = incomming_packet.data[1];
			cl_ship_state.ship_size_index = incomming_packet.data[2];
			cl_ship_state.horizontal = incomming_packet.data[3];

			outgoing_packet.type = PACKET_SHIP_PLACE_RESPONSE;
			outgoing_packet.data[0] = PlaceShip(&cl_ship_state, &enemy_placement_state);

			SendPacket();
			break;
		case PACKET_SHIP_REMOVE_REQUEST:

			cl_ship_state.x = incomming_packet.data[0];
			cl_ship_state.y = incomming_packet.data[1];
			cl_ship_state.ship_size_index = incomming_packet.data[2];
			cl_ship_state.horizontal = incomming_packet.data[3];

			outgoing_packet.type = PACKET_SHIP_REMOVE_RESPONSE;
			RemoveShip(&cl_ship_state, &enemy_placement_state, false);
			SendPacket();

			break;
		case PACKET_READY_REQUEST:
			enemy_placement_state.ready = true;
			outgoing_packet.type = PACKET_READY_RESPONSE;
			outgoing_packet.data[0] = my_placement_state.ready;
			SendPacket();
			break;
		case PACKET_READY_RESPONSE:
			enemy_placement_state.ready = incomming_packet.data[0];
			EndWaitRemote();
			break;
		case PACKET_START_GAME_RESPONSE:
			game_state = STATE_MAIN_GAME;
			my_turn = true;
			EndWaitRemote();
			break;
		default:
			break;
		}

	}
}

void ServerHandlePlacement()
{
	if (my_placement_state.ready)
	{
		if (enemy_placement_state.ready && !wait_remote)
		{
			outgoing_packet.type = PACKET_START_GAME_REQUEST;
			SendPacket();
			StartWaitRemote();
		}
		return;
	}

	
	if (isKeyJustPressed(SDL_SCANCODE_BACKSPACE))
	{
		RemoveShip(&ship_placement_state, &my_placement_state, true);
	}
	if (isKeyJustPressed(SDL_SCANCODE_RETURN))
	{
		if (my_placement_state.ships_count == MAX_SHIPS)
		{
			my_placement_state.ready = true;
			outgoing_packet.type = PACKET_READY_REQUEST;
			SendPacket();
			StartWaitRemote();
		}
		else if (!PlaceShip(&ship_placement_state, &my_placement_state))
		{
			bg_blink_counter = 5;
			warning_counter = 100;
		}
	}
}

void ClientHandleServerResponse()
{
	while (Check_Packet())
	{
		Pop_Packet();

		switch (incomming_packet.type)
		{
		case PACKET_SHIP_PLACE_RESPONSE:
			if (incomming_packet.data[0])
			{
				PlaceShip(&ship_placement_state, &my_placement_state);
			}
			else
			{
				bg_blink_counter = 5;
				warning_counter = 100;
			}
			EndWaitRemote();
			break;
		case PACKET_SHIP_REMOVE_RESPONSE:
			RemoveShip(&ship_placement_state, &my_placement_state, true);
			EndWaitRemote();
			break;
		case PACKET_READY_REQUEST:
			enemy_placement_state.ready = true;
			outgoing_packet.type = PACKET_READY_RESPONSE;
			outgoing_packet.data[0] = my_placement_state.ready;
			SendPacket();
			break;
		case PACKET_READY_RESPONSE:
			enemy_placement_state.ready = incomming_packet.data[0];
			EndWaitRemote();
			break;
		case PACKET_START_GAME_REQUEST:
			game_state = STATE_MAIN_GAME;
			my_turn = false;
			outgoing_packet.type = PACKET_START_GAME_RESPONSE;
			SendPacket();
			break;
		default:
			break;
		}
	}
}


void ClientHandlePlacement()
{

	if (isKeyJustPressed(SDL_SCANCODE_RETURN))
	{

		if (my_placement_state.ships_count == MAX_SHIPS && !my_placement_state.ready)
		{
			my_placement_state.ready = true;
			outgoing_packet.type = PACKET_READY_REQUEST;
		}
		else
		{
			outgoing_packet.type = PACKET_SHIP_PLACE_REQUEST;
			outgoing_packet.data[0] = ship_placement_state.x;
			outgoing_packet.data[1] = ship_placement_state.y;
			outgoing_packet.data[2] = ship_placement_state.ship_size_index;
			outgoing_packet.data[3] = ship_placement_state.horizontal;
		}
		SendPacket();
		StartWaitRemote();
	}
	if (isKeyJustPressed(SDL_SCANCODE_BACKSPACE))
	{
		outgoing_packet.type = PACKET_SHIP_REMOVE_REQUEST;

		outgoing_packet.data[0] = ship_placement_state.x;
		outgoing_packet.data[1] = ship_placement_state.y;
		outgoing_packet.data[2] = ship_placement_state.ship_size_index;
		outgoing_packet.data[3] = ship_placement_state.horizontal;

		SendPacket();
		StartWaitRemote();

	}

}

void HandleShipPlacement()
{
	if (server_mode)
	{
		ServerHandlePlacement();
		ServerHandleClientPlacement();
	}
	else
	{
		ClientHandleServerResponse();
		ClientHandlePlacement();
	}
}


void HandleShipMovement()
{
	ship_placement_state.x = my_cursor_x;
	ship_placement_state.y = my_cursor_y;
	if (isKeyJustPressed(SDL_SCANCODE_R))
	{
		ship_placement_state.horizontal = !ship_placement_state.horizontal;
	}

	if (isKeyJustPressed(SDL_SCANCODE_Q))
	{
		PrevShipSize(&ship_placement_state, &my_placement_state);
	}
	if (isKeyJustPressed(SDL_SCANCODE_E))
	{
		printf("NextShipSize called from %s\n", __func__);
		NextShipSize(&ship_placement_state, &my_placement_state);
	}
}


void TickPlacingShips()
{

	if (bg_blink_counter)
		Video_Clear(1);
	else
		Video_Clear(0);

	Video_PlacingShips();

	if (UpdateWaitRemote())
		return;

	if (wait_remote)
	{
		Video_DrawWaitRemote();
	}
	else
	{
		HandleShipMovement();
		HandleCursorMovement();
	}

	HandleShipPlacement();
}

void TickError()
{
	Video_Error();
}

void TickVideoTest()
{
	Video_Test();
}

void Client_HandleShootRequest()
{

	ship_t* s = NULL;
	cellstate_t state = incomming_packet.data[0];

	enemy_cursor_x = incomming_packet.data[1];
	enemy_cursor_y = incomming_packet.data[2];

	if (state == CELL_SUNK)
	{
		s             = enemy_placement_state.ships; // юзаем не используемую память
		s->x          =    incomming_packet.data[3];
		s->y          =    incomming_packet.data[4];
		s->size       =    incomming_packet.data[5];
		s->horizontal =    incomming_packet.data[6];	
	}
	
	ProcessReceivedCell(&my_placement_state, state, s, enemy_cursor_x, enemy_cursor_y);

	outgoing_packet.type = PACKET_SHOOT_RESPONSE;
	SendPacket();
	EndWaitRemote();
}

void Server_HandleShootRequest()
{
	ship_t* s;
	fieldcell_t cell;

	enemy_cursor_x = incomming_packet.data[0];
	enemy_cursor_y = incomming_packet.data[1];

	cell = ShootToField(&my_placement_state, enemy_cursor_x, enemy_cursor_y);

	outgoing_packet.type = PACKET_SHOOT_RESPONSE;
	outgoing_packet.data[0] = cell.state;
	outgoing_packet.data[1] = enemy_cursor_x;
	outgoing_packet.data[2] = enemy_cursor_y;

	if (cell.state == CELL_SUNK)
	{
		s = my_placement_state.ships + cell.ship_index;
		outgoing_packet.data[3] = s->x;
		outgoing_packet.data[4] = s->y;
		outgoing_packet.data[5] = s->size;
		outgoing_packet.data[6] = s->horizontal;
	}
	else
	{
		need_switch = cell.state == CELL_MISS;
	}

	SendPacket();
	EndWaitRemote();
}

void HandleShootRequest()
{
	if (server_mode)
	{
		Server_HandleShootRequest();
	}
	else
	{
		Client_HandleShootRequest();
	}

}

void Server_HandleShootResponse()
{
	EndWaitRemote();
}

void Client_HandleShootResponse()
{
	ship_t* s = NULL;
	cellstate_t state = incomming_packet.data[0];
	enemy_cursor_x = incomming_packet.data[1];
	enemy_cursor_y = incomming_packet.data[2];

	if (state == CELL_SUNK)
	{
		s = enemy_placement_state.ships;
		s->x = incomming_packet.data[3];
		s->y = incomming_packet.data[4];
		s->size = incomming_packet.data[5];
		s->horizontal = incomming_packet.data[6];
	}

	ProcessReceivedCell(&enemy_placement_state, state, s, enemy_cursor_x, enemy_cursor_y);
	EndWaitRemote();
}

void HandleShootResponse()
{
	if (server_mode)
	{
		Server_HandleShootResponse();
	}
	else
	{
		Client_HandleShootResponse();
	}
}

void HandleTurnRequest()
{
	my_turn = !my_turn;
	outgoing_packet.type = PACKET_TURN_SWITCH_RESPONSE;
	SendPacket();
}

void HandleTurnResponse()
{
	my_turn = !my_turn;
	EndWaitRemote();
}


void HandleEndGameRequest()
{
	game_state;
}
void HandleEndGameResponse()
{

}

void HandleRemoteInput()
{
	fieldcell_t cell;
	bool local_server_mode;

	while (Check_Packet())
	{
		Pop_Packet();

		switch (incomming_packet.type)
		{
		case PACKET_CURSOR_POS:
			enemy_cursor_x = incomming_packet.data[0];
			enemy_cursor_y = incomming_packet.data[1];
			break;
		case PACKET_SHOOT_REQUEST:
			HandleShootRequest();
			break;
		case PACKET_SHOOT_RESPONSE:
			HandleShootResponse();
			break;
		case PACKET_TURN_SWITCH_REQUEST:
			HandleTurnRequest();
			break;
		case PACKET_TURN_SWITCH_RESPONSE:
			HandleTurnResponse();
			break;
		case PACKET_END_GAME_REQUEST:
			HandleEndGameRequest();
			break;
		case PACKET_END_GAME_RESPONSE:
			HandleEndGameResponse();
			break;
		default:
			break;
		}
	}
}

void ServerShoot()
{
	fieldcell_t cell;
	ship_t* s;

	cell = ShootToField(&enemy_placement_state, my_cursor_x, my_cursor_y);

	outgoing_packet.data[0] = cell.state;  
	outgoing_packet.data[1] = my_cursor_x;
	outgoing_packet.data[2] = my_cursor_y;

	if (cell.state == CELL_SUNK)
	{
		s = enemy_placement_state.ships + cell.ship_index;
		outgoing_packet.data[3] = s->x;
		outgoing_packet.data[4] = s->y;
		outgoing_packet.data[5] = s->size;
		outgoing_packet.data[6] = s->horizontal;
	}
	else
	{
		need_switch = cell.state == CELL_MISS;
	}
}

void SendMyCursorToEnemy()
{
	if (isKeyJustPressed(SDL_SCANCODE_RETURN))
	{

		outgoing_packet.type = PACKET_SHOOT_REQUEST;
		outgoing_packet.data[0] = my_cursor_x;
		outgoing_packet.data[1] = my_cursor_y;

		if (server_mode)
		{
			ServerShoot();
		}

		StartWaitRemote();
		SendPacket();
	}
	else if (!cursor_send_counter)
	{
		outgoing_packet.type    = PACKET_CURSOR_POS;
		outgoing_packet.data[0] = my_cursor_x;
		outgoing_packet.data[1] = my_cursor_y;
		SendPacket();
	}
}

void HandleMyTurnInput()
{
	HandleCursorMovement();
	SendMyCursorToEnemy();
}

void TickMainGame()
{
	if (UpdateWaitRemote())
		return;

	if (bg_blink_counter)
	{
		Video_Clear(1);
	}
	else
	{
		Video_Clear(0);
	}

	if (my_turn)
	{
		Video_DrawMyTurn();
	}
	else
	{
		Video_DrawEnemyTurn();
	}

	if (wait_remote)
	{
		Video_DrawWaitRemote();
	}
	else
	{
		if (all_ships_destroyed)
		{
			outgoing_packet.type = PACKET_END_GAME_REQUEST;
			SendPacket();
			StartWaitRemote();
		}
		else if (need_switch)
		{
			need_switch = false;
			outgoing_packet.type = PACKET_TURN_SWITCH_REQUEST;
			SendPacket();
			StartWaitRemote();
		}
		else
		{
			if (my_turn)
			{
				HandleMyTurnInput();
			}
		}
	}

	HandleRemoteInput();
}

void Tick()
{
	Net_Update();
	Input_Update();

	switch (game_state)
	{
	case STATE_VIDEO_TEST:
		TickVideoTest();
		break;
	case STATE_CHOOSE_MODE:
		TickChooseMode();
		break;
	case STATE_WAIT_FOR_CONNECTION:
		TickWaitForConnection();
		break;
	case STATE_PLACING_SHIPS:
		TickPlacingShips();
		break;
	case STATE_MAIN_GAME:
		TickMainGame();
		break;
	case STATE_ERROR:
		TickError();
		break;
	default:
		printf("What the fuck is this state?\n");
		running = false;
		return;
	}



	Video_Render();
	UpdateCounters();
	SDL_Delay(16);
}

int main(int argc, char* argv[])
{
	Init();
	while (running)
	{
		Tick();
	}
	Quit();
	return EXIT_SUCCESS;
}