#include "CWorld.h"
#include "../utils/Memory/CMemory.h"
#include "../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>
#include <pthread.h>

static constexpr unsigned int gamePlayerSlots = 2;
static bool playerSlotsVerified = false;

void CWorld::Initialise()
{
    CNecessary::Log(OBFUSCATE("CWorld Initialise"));

    playerSlotsVerified = CMemory::HasMappedBytes(SA_ADDR(0x190C1C), "\x8A\x2D\x00\xB0\x4A\xA1\x29\x91\x0B\x3B\x80\x52\x29\x29\xAB\x9B", 16, "__text");
    if (!playerSlotsVerified)
    {
        CNecessary::Log(OBFUSCATE("CWorld: iOS player slots unavailable"));
    }
}

CPedGta* CWorld::GetPlayerPed(unsigned int slot)
{
    if (slot >= eMaxItem::MAX_PLAYERS || !pthread_main_np()) {
        return nullptr;
    }

    if (slot < gamePlayerSlots)
    {
        // สองช่องแรกอ่านจากตารางที่ GTA iOS จัดการ
        return playerSlotsVerified ? reinterpret_cast<const CPlayerInfo*>(SA_ADDR(0x741A68) + _dyld_get_image_vmaddr_slide(0))[slot].m_pPed : nullptr;
    }

    return CWorld::Players[slot].m_pPed;
}

int32_t CWorld::FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed)
{
    if (!pPlayersPed || !pthread_main_np()) {
        return -1;
    }

    for (unsigned int i = 0; i < gamePlayerSlots; ++i)
    {
        if (CWorld::GetPlayerPed(i) == pPlayersPed)
        {
            return static_cast<int32_t>(i);
        }
    }

    for (int i = gamePlayerSlots; i < eMaxItem::MAX_PLAYERS; ++i)
    {
        if (CWorld::Players[i].m_pPed == pPlayersPed)
        {
            return i;
        }
    }

    return -1;
}
