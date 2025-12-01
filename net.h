#pragma once

typedef enum PACKET_TYPE 
{
    PACKET_CONNECTION_REQUEST,
    PACKET_CONNECTION_ACCEPTED,

    PACKET_READY_REQUEST,
    PACKET_READY_RESPONSE,

    PACKET_START_GAME_REQUEST,
    PACKET_START_GAME_RESPONSE,

    PACKET_END_GAME_REQUEST,
    PACKET_END_GAME_RESPONSE,

    PACKET_SHIP_PLACE_REQUEST,
    PACKET_SHIP_PLACE_RESPONSE,

    PACKET_SHIP_REMOVE_REQUEST,
    PACKET_SHIP_REMOVE_RESPONSE,

    PACKET_TURN_SWITCH_REQUEST,
    PACKET_TURN_SWITCH_RESPONSE,

    PACKET_HEARTBEAT_REQUEST,
    PACKET_HEARTBEAT_RESPONSE,

    PACKET_CURSOR_POS,

    PACKET_SHOOT_REQUEST,
    PACKET_SHOOT_RESPONSE,

    PACKET_REMOTE_ERROR,
} PACKET_TYPE;

#pragma pack(push, 1)
typedef struct packet_s
{
    Uint8 hash;
    Uint8 type;
    Uint16 data[7];
} packet_t;
#pragma pack(pop)

extern packet_t incomming_packet;
extern packet_t outgoing_packet;

#define PACKET_GET_TYPE(pkt) ((byte)((pkt)->header & 0xFF))
#define PACKET_GET_HASH(pkt) ((byte)(((pkt)->header >> 8) & 0xFF))
#define PACKET_SET_TYPE(pkt, t) ((pkt)->header = ((pkt)->header & 0xFF00) | ((byte)(t)))
#define PACKET_SET_HASH(pkt, h) ((pkt)->header = ((pkt)->header & 0x00FF) | (((byte)(h)) << 8))

bool Net_Init();
void Net_Quit();
void Net_Update();

// на плате такого не будет
bool Net_SetMode(bool server_mode);
void Net_CloseSocket();

void Pop_Packet();
bool Check_Packet();
void SendPacket();