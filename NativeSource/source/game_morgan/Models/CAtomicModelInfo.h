#pragma once

#include "CBaseModelInfo.h"

class CAtomicModelInfo : public CBaseModelInfo
{
};

static_assert(sizeof(CAtomicModelInfo) == 0x48);