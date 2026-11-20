#pragma once

#include "CClumpModelInfo.h"

class CPedModelInfo : public CClumpModelInfo
{
private:
    [[maybe_unused]] std::byte pedStateBeforeStats[0x58 - 0x50];

public:
    unsigned int m_defaultPedStats; // 0x58, verified in CPed::SetModelIndex on iOS.

private:
    [[maybe_unused]] std::byte pedStateAfterStats[0x78 - 0x5C];
};

static_assert(sizeof(CPedModelInfo) == 0x78);