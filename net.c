#include "main.h"

packet_t incomming_packet;
packet_t outgoing_packet;

#define QUEUE_SIZE 128
#define SERVER_PORT 8080

static int incoming_front_ptr = 0;
static int incoming_back_ptr = 0;
static int incoming_packet_count = 0;
static packet_t incoming_queue[QUEUE_SIZE];

UDPsocket udp_socket = NULL;
UDPpacket* incoming_udp = NULL;
UDPpacket* outgoing_udp = NULL;

const char* remote_address_str = "127.0.0.1";
IPaddress remote_address;
bool is_server_mode = false;
bool connection_established = false;


static bool isFull()
{
    return incoming_packet_count == QUEUE_SIZE;
}

static bool isEmpty()
{
    return incoming_packet_count == 0;
}

bool Check_Packet()
{
    bool has_packets = incoming_packet_count > 0;
    //printf("Check_Packet: count=%d, front=%d, back=%d\n", incoming_packet_count, incoming_front_ptr, incoming_back_ptr);
    return has_packets;
}

void Pop_Packet()
{
    printf("Before Pop: count=%d, front=%d, back=%d\n",
        incoming_packet_count, incoming_front_ptr, incoming_back_ptr);

    if (isEmpty())
    {
        printf("Pop_Packet: queue empty\n");
        return;
    }

    incomming_packet = incoming_queue[incoming_front_ptr];
    incoming_front_ptr = (incoming_front_ptr + 1) % QUEUE_SIZE;
    incoming_packet_count--;

    printf("After Pop: count=%d, front=%d, back=%d, packet_type=%d\n",
        incoming_packet_count, incoming_front_ptr, incoming_back_ptr, incomming_packet.type);
}

static void PushPacket(packet_t* p)
{
    printf("Before Push: count=%d, front=%d, back=%d\n",
        incoming_packet_count, incoming_front_ptr, incoming_back_ptr);

    if (isFull())
    {
        printf("Queue full, packet dropped\n");
        return;
    }

    incoming_queue[incoming_back_ptr] = *p;
    incoming_back_ptr = (incoming_back_ptr + 1) % QUEUE_SIZE;
    incoming_packet_count++;

    printf("After Push: count=%d, front=%d, back=%d, packet_type=%d\n",
        incoming_packet_count, incoming_front_ptr, incoming_back_ptr, p->type);
}

uint8_t CalculateCRC8(packet_t* packet) {
    uint8_t crc = 0x00;
    uint8_t poly = 0x07;
    crc ^= packet->type;
    for (int j = 0; j < 8; j++) {
        if (crc & 0x80)
            crc = (crc << 1) ^ poly;
        else
            crc <<= 1;
    }
    uint8_t* data_bytes = (uint8_t*)packet->data;
    for (int i = 0; i < 14; i++) {
        crc ^= data_bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ poly;
            else
                crc <<= 1;
        }
    }

    return crc;
}

bool ValidatePacket(packet_t* packet) {
    uint8_t original_hash = packet->hash;

    packet->hash = 0;
    uint8_t calculated_hash = CalculateCRC8(packet);
    packet->hash = original_hash;

    return (original_hash == calculated_hash);
}

void SendPacket()
{
    if (!udp_socket)
    {
        printf("Cannot send - socket not open\n");
        return;
    }

    outgoing_packet.hash = 0;
    outgoing_packet.hash = CalculateCRC8(&outgoing_packet);

    SDL_memcpy(outgoing_udp->data, &outgoing_packet, sizeof(packet_t));
    outgoing_udp->len = sizeof(packet_t);
    outgoing_udp->address = remote_address;
    outgoing_udp->channel = -1;

    if (SDLNet_UDP_Send(udp_socket, -1, outgoing_udp) == 0)
    {
        printf("Failed to send packet: %s\n", SDLNet_GetError());
    }
    else {
        printf("Succesefuly sent\n");
    }
}

void Net_CloseSocket()
{
    if (udp_socket)
        SDLNet_UDP_Close(udp_socket);
    udp_socket = NULL;
    connection_established = false;
}

bool Net_SetMode(bool server_mode)
{
    Net_CloseSocket();
    is_server_mode = server_mode;
    if (server_mode)
    {
        udp_socket = SDLNet_UDP_Open(SERVER_PORT);
        if (udp_socket)
        {
            printf("Server mode started on port %d\n", SERVER_PORT);
        }
    }
    else
    {
        udp_socket = SDLNet_UDP_Open(0);
        if (udp_socket)
        {
            if (SDLNet_ResolveHost(&remote_address, remote_address_str, SERVER_PORT) == 0)
            {
                connection_established = true;
                printf("Client mode connected to %s:%d\n", remote_address_str, SERVER_PORT);
                return true;
            }
            else
            {
                printf("Can't resolve server %s:%d\n", remote_address_str, SERVER_PORT);
                Net_CloseSocket();
                return false;
            }
        }
        return false;
    }
    return udp_socket != NULL;
}

void Net_Update()
{
    if (!udp_socket)
        return;

    while (SDLNet_UDP_Recv(udp_socket, incoming_udp))
    {
        printf("Recieved ");
        if (incoming_udp->len != sizeof(packet_t)) 
        {
            printf("bad packet\n");
            continue;
        }

        packet_t* received_packet = (packet_t*)incoming_udp->data;

        if (!ValidatePacket(received_packet))
        {
            printf("invalid packet CRC\n");
            continue;
        }

        if (!connection_established)
        {
            connection_established = true;
            remote_address = incoming_udp->address;
        }
        printf("from %s\n", SDLNet_ResolveIP(&incoming_udp->address));
        PushPacket(received_packet);
    }
}

bool Net_Init()
{
    if (SDLNet_Init() < 0)
    {
        printf("Can't init SDLNet: %s\n", SDLNet_GetError());
        return false;
    }

    incoming_udp = SDLNet_AllocPacket(sizeof(packet_t));
    outgoing_udp = SDLNet_AllocPacket(sizeof(packet_t));

    if (!incoming_udp || !outgoing_udp)
    {
        printf("Can't allocate UDP packets: %s\n", SDLNet_GetError());
        return false;
    }

    printf("Network module initialized\n");
    return true;
}

void Net_Quit()
{
    Net_CloseSocket();
    if (outgoing_udp)
        SDLNet_FreePacket(outgoing_udp);
    if (incoming_udp)
        SDLNet_FreePacket(incoming_udp);

    outgoing_udp = NULL;
    incoming_udp = NULL;

    SDLNet_Quit();
}