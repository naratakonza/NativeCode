#pragma once

#include "CBaseModelInfo.h"

class CClumpModelInfo : public CBaseModelInfo
{
private:
    [[maybe_unused]] std::byte clumpState[0x50 - 0x48];
};

static_assert(sizeof(CClumpModelInfo) == 0x50);