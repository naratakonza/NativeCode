#include "CWorld.h"
#include "../utils/Memory/CMemory.h"
#include "../utils/Necessary/CNecessary.h"

uint8_t CWorld::PlayerInFocus;

void CWorld::Initialise()
{
    CNecessary::Log(OBFUSCATE("CWorld Initialise"));

    if (!CMemory::HasMappedInstruction(SA_ADDR(0x18CBB0), "\xBC\x29\x0C\x14") || !CMemory::HasMappedBytes(SA_ADDR(0x4972A0), "\xF0\x00\x00\xD0\x10\x5E\x42\xF9\x00\x02\x1F\xD6", 12, "__mg_gameproc"))
    {
        CNecessary::Log(OBFUSCATE("CWorld: iOS player slot redirect unavailable"));
    }
}

int32_t CWorld::FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed)
{
    for (int i = 0; i < eMaxItem::MAX_PLAYERS; ++i)
    {
        if (CWorld::Players[i].m_pPed == pPlayersPed)
        {
            return i;
        }
    }

    return -1;
}
