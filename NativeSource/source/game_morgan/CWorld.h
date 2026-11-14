#pragma once

#include "CPlayerInfo.h"
#include "Enums/eMaxItem.h"

class CWorld
{
public:
    __attribute__((visibility("default"))) static inline CPlayerInfo Players[eMaxItem::MAX_PLAYERS];
    __attribute__((visibility("default"))) static uint8_t PlayerInFocus;

public:
    static void Initialise();
    __attribute__((visibility("default"))) static int32_t FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed);
};
