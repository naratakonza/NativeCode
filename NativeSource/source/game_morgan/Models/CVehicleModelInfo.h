#pragma once

#include "CClumpModelInfo.h"

class CVehicleModelInfo : public CClumpModelInfo
{
private:
    [[maybe_unused]] std::byte vehicleState[0x6B0 - 0x50];
};

static_assert(sizeof(CVehicleModelInfo) == 0x6B0);