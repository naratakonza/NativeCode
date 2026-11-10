#pragma once

#include "../Core/CVector.h"
#include "../utils/Quat/CQuaternion.h"

struct RpHAnimBlendInterpFrame
{
    CQuaternion orientation;
    CVector     translation;
};
static_assert(sizeof(RpHAnimBlendInterpFrame) == 0x1C);