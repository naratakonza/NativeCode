#pragma once

#include "Core/CVector.h"

class CSprite
{
public:
    static bool CalcScreenCoors(const CVector* In, CVector* pResult, float* pScaleX, float* pScaleY, bool ClipFarPlane, bool ClipNearPlane);
};
