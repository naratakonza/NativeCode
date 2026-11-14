#include "CPlayerPedSlot.h"
#include "../../Enums/eMaxItem.h"

std::unordered_map<uint8_t, uint8_t> CPlayerPedSlot::m_playerSlots;

void CPlayerPedSlot::Set(uint8_t byteSlot, uint8_t byteState)
{
    if (byteSlot >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    if (byteState == 0)
    {
        CPlayerPedSlot::m_playerSlots.erase(byteSlot);
    }
    else
    {
        CPlayerPedSlot::m_playerSlots[byteSlot] = byteState;
    }
}

uint8_t CPlayerPedSlot::IsActive(uint8_t byteSlot)
{
    if (byteSlot >= eMaxItem::PLAYER_PED_SLOTS) {
        return 0;
    }

    auto playerIter = CPlayerPedSlot::m_playerSlots.find(byteSlot);
    if (playerIter != CPlayerPedSlot::m_playerSlots.end())
	{
        return playerIter->second;
    }

    return 0;
}