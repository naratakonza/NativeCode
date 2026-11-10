#pragma once

#include "CClumpModelInfo.h"

class CPedModelInfo : public CClumpModelInfo
{
private:
    [[maybe_unused]] std::byte pedState[0x78 - 0x50];
};

static_assert(sizeof(CPedModelInfo) == 0x78);