#include "CPedPointerRecorder.h"
#include "../../../utils/Necessary/CNecessary.h"

std::unordered_map<uint8_t, uintptr_t> CPedPointerRecorder::m_PlayerPedPtrs;
std::unordered_map<uintptr_t, uint8_t> CPedPointerRecorder::m_PedPtrToNum;

void CPedPointerRecorder::Record(uint8_t byteNum, uintptr_t pointer)
{
	if (byteNum >= eMaxItem::PLAYER_PED_SLOTS) {
        CNecessary::Log(OBFUSCATE("CPedPointerRecorder::Record: byteNum = %d"), byteNum);
		return;
	}

    auto playerIter = CPedPointerRecorder::m_PlayerPedPtrs.find(byteNum);
    uintptr_t oldPointer = (playerIter != CPedPointerRecorder::m_PlayerPedPtrs.end()) ? playerIter->second : 0;

    if (pointer == 0)
	{
        if (oldPointer != 0)
		{
            CPedPointerRecorder::m_PedPtrToNum.erase(oldPointer);
        }

        if (playerIter != CPedPointerRecorder::m_PlayerPedPtrs.end())
		{
            CPedPointerRecorder::m_PlayerPedPtrs.erase(playerIter);
        }

        return;
    }

    if (oldPointer != pointer)
	{
        if (oldPointer != 0)
		{
            CPedPointerRecorder::m_PedPtrToNum.erase(oldPointer);
        }

        CPedPointerRecorder::m_PlayerPedPtrs[byteNum] = pointer;
        CPedPointerRecorder::m_PedPtrToNum[pointer] = byteNum;
    }
}

uint8_t CPedPointerRecorder::FindPlayerNumFromPointer(uintptr_t dwPedPtr)
{
	if (dwPedPtr == 0) {
		return 0;
	}

	auto playerIter = CPedPointerRecorder::m_PedPtrToNum.find(dwPedPtr);
    if (playerIter != CPedPointerRecorder::m_PedPtrToNum.end())
	{
        return playerIter->second;
    }

    return 0;
}

uintptr_t CPedPointerRecorder::FindPlayerPointerFromNum(uint8_t num)
{
	auto playerIter = CPedPointerRecorder::m_PlayerPedPtrs.find(num);
    if (playerIter != CPedPointerRecorder::m_PlayerPedPtrs.end())
	{
        return playerIter->second;
    }

    return 0;
}