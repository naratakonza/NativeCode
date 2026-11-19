#pragma once

#include "../../Core/CVector.h"
#include "../../Entity/CEntityGta.h"

class CTaskSimpleJetPack
{
public:
    static void Constructor(uintptr_t thiz, const CVector* pVecTargetPos, float fCruiseHeight, int nHoverTime, CEntityGta* pTargetEnt);
    static void Destructor(uintptr_t thiz);
};