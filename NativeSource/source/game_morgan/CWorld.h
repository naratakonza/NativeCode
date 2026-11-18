#pragma once

#include "CPlayerInfo.h"
#include "Enums/eMaxItem.h"

struct CEntityGta;

struct PlayerTable
{
    CPlayerInfo* players;
    unsigned int slots;
};

class CWorld
{
public:
    static CPlayerInfo* Players;
    static uint8_t& PlayerInFocus;

public:
    static void Initialise();
    static void AddEntity(CEntityGta* entity);
    static void RemoveEntity(CEntityGta* entity);
    static void RemovePlayerPed(unsigned int slot);
    static CPedGta* GetPlayerPed(unsigned int slot);
    __attribute__((visibility("default"))) static int32_t FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed);
};