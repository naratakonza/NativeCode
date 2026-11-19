#include "CWorld.h"
#include "../utils/Memory/CMemory.h"
#include "../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>
#include <pthread.h>

CPlayerInfo* CWorld::Players = nullptr;
uint8_t& CWorld::PlayerInFocus = *reinterpret_cast<uint8_t*>(SA_ADDR(0x741E18) + _dyld_get_image_vmaddr_slide(0));

static const PlayerTable& GetPlayerTable()
{
    static const PlayerTable table = []() -> PlayerTable
    {
        const auto slide = _dyld_get_image_vmaddr_slide(0);
        if (CMemory::HasMappedBytes(SA_ADDR(0x190C1C), "\x6A\x48\x00\x90\x4A\x01\x00\x91\x0B\x3B\x80\x52\x29\x29\xAB\x9B", 16, "__text"))
        {
            return {reinterpret_cast<CPlayerInfo*>(SA_ADDR(0xA9C000) + slide), eMaxItem::MAX_PLAYERS};
        }

        if (CMemory::HasMappedBytes(SA_ADDR(0x190C1C), "\x8A\x2D\x00\xB0\x4A\xA1\x29\x91\x0B\x3B\x80\x52\x29\x29\xAB\x9B", 16, "__text"))
        {
            return {reinterpret_cast<CPlayerInfo*>(SA_ADDR(0x741A68) + slide), 2};
        }

        return {};
    }();

    return table;
}

void CWorld::Initialise()
{
    CNecessary::Log(OBFUSCATE("CWorld Initialise"));

    CWorld::Players = GetPlayerTable().players;
    if (!CWorld::Players)
    {
        CNecessary::Log(OBFUSCATE("CWorld: iOS player slots unavailable"));
    }
}

void CWorld::AddEntity(CEntityGta* entity)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1826BC), "\x08\x10\x40\xF9\x28\x01\x00\xB4", 8, "__text");
    if (verified && entity && pthread_main_np())
    {
        reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x1826AC) + _dyld_get_image_vmaddr_slide(0))(entity);
    }
}

void CWorld::RemoveEntity(CEntityGta* entity)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x182538), "\xF3\x03\x00\xAA\x08\x00\x40\xF9", 8, "__text");
    if (verified && entity && pthread_main_np())
    {
        reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x18252C) + _dyld_get_image_vmaddr_slide(0))(entity);
    }
}

void CWorld::RemovePlayerPed(unsigned int slot)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x26D07C), "\xC9\xA8\x08\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x26D0F8), "\xAF\xA8\x08\x14");
    if (verified && slot >= 2 && slot < eMaxItem::PLAYER_PED_SLOTS && CWorld::GetPlayerPed(slot))
    {
        reinterpret_cast<void (*)(unsigned int)>(SA_ADDR(0x26D07C) + _dyld_get_image_vmaddr_slide(0))(slot);
    }
}

CPedGta* CWorld::GetPlayerPed(unsigned int slot)
{
    if (!pthread_main_np()) {
        return nullptr;
    }

    const auto& table = GetPlayerTable();
    return slot < table.slots ? table.players[slot].m_pPed : nullptr;
}

int32_t CWorld::FindPlayerSlotWithPedPointer(CPedGta* pPlayersPed)
{
    if (!pPlayersPed) {
        return -1;
    }

    const auto& table = GetPlayerTable();
    for (unsigned int i = 0; i < table.slots; ++i)
    {
        if (table.players[i].m_pPed == pPlayersPed)
        {
            return static_cast<int32_t>(i);
        }
    }

    return -1;
}