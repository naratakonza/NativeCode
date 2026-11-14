#pragma once

#include <cstdint>
#include <unordered_map>

class CPlayerPedSlot {
private:
	static std::unordered_map<uint8_t, uint8_t> m_playerSlots;

public:
	static void Set(uint8_t, uint8_t);
	static uint8_t IsActive(uint8_t byteSlot);
};