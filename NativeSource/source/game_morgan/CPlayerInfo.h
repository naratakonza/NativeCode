#pragma once

#include <cstddef>
#include <cstdint>

struct CPedGta;

class CPlayerInfo
{
public:
    CPedGta *m_pPed;
    uint8_t skip[464];
};
static_assert(sizeof(CPlayerInfo) == 0x1D8);
static_assert(offsetof(CPlayerInfo, m_pPed) == 0);