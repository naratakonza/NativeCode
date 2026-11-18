#pragma once

#include "../RenderWare/rwplcore.h"

class CVector2D : public RwV2d {
public:
    CVector2D() = default;
    CVector2D(float x, float y) : RwV2d{ x, y } { }
};
static_assert(sizeof(CVector2D) == 0x8);