#pragma once

#include <cstdint>
#include <unordered_map>
#include "../../Enums/eMaxItem.h"

class CPedPointerRecorder
{
private:
	static std::unordered_map<uint8_t, uintptr_t> m_PlayerPedPtrs;
    static std::unordered_map<uintptr_t, uint8_t> m_PedPtrToNum;

public:
	static void Record(uint8_t, uintptr_t);
	static uint8_t FindPlayerNumFromPointer(uintptr_t);
	static uintptr_t FindPlayerPointerFromNum(uint8_t);
};