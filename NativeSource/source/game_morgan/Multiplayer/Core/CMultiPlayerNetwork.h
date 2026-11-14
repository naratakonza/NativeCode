#pragma once

#include <raknet/NetworkTypes.h>
#include <raknet/PacketEnumerations.h>

class CMultiPlayerNetwork
{
public:
	static void Update();
    static void Packet_AuthKey(Packet*);
    static void Packet_ConnectionSucceeded(Packet*);
    static void Packet_PlayerSync(Packet*);
    static void Packet_VehicleSync(Packet*);
    static void Packet_PassengerSync(Packet*);
    static void Packet_MarkersSync(Packet*);
    static void Packet_TrailerSync(Packet *pkt);
    static void Packet_UnoccupiedSync(Packet *pkt);
    static void Packet_BulletSync(Packet*);
    static void Packet_AimSync(Packet *pkt);
    static void Packet_Bifrost(Packet *pkt);
    static unsigned char GetPacketID(Packet*);
};