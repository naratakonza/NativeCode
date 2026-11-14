#pragma once

#include "CPlayerInfo.h"
#include "Enums/eMaxItem.h"

class CWorld
{
public:
    static inline CPlayerInfo Players[eMaxItem::MAX_PLAYERS];

public:
    static void Initialise();
    static CPedGta* GetPlayerPed(unsigned int slot);
    static int32_t FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed);
};
