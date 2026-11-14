#pragma once

#include "../CSimpleTransform.h"
#include "../Core/CMatrix.h"

struct CPlaceableGta
{
    uintptr_t vtable;
    CSimpleTransform m_transform;
    CMatrix *mat;
};
static_assert(sizeof(CPlaceableGta) == 0x20);
static_assert(offsetof(CPlaceableGta, m_transform) == 0x08);
static_assert(offsetof(CPlaceableGta, mat) == 0x18);