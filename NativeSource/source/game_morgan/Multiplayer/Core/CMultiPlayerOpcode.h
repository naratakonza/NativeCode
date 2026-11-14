#pragma once

#include <cstdint>
#include <raknet/BitStream.h>
#include <raknet/NetworkTypes.h>
#include <raknet/RakClientInterface.h>
#include <raknet/PacketEnumerations.h>

#define SPECTATE_TYPE_NORMAL	1
#define SPECTATE_TYPE_FIXED		2
#define SPECTATE_TYPE_SIDE		3

class CMultiPlayerOpcode
{
public:
	static bool Receive(RakNet::BitStream*, uint8_t);
};