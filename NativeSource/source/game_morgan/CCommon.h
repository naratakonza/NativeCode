#pragma once

#include "Entity/CEntity.h"
#include "Entity/Ped/CPedGta.h"
#include "Models/CBaseModelInfo.h"

class CCommon
{
public:
	static uintptr_t GetModelInfoByID(int iModelID);
	static bool IsGameEntityArePlaceable(CEntityGta *pEntity);
    static bool IsGameEntityRenderable(CEntityGta *pEntity);
    static bool IsValidGamePed(CPedGta *pPed);
    static bool IsPedModel(unsigned int iModelID);
};