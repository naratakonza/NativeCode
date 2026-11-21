#pragma once

#include "Core/CVector.h"
#include "Entity/CEntityGta.h"
#include "Enums/eMaxPool.h"

class CBuildingRemove
{
public:
    static int RemovedModelIDs[eMaxPool::MAX_REMOVE_BUILDINGS];
    static CVector RemovedPositions[eMaxPool::MAX_REMOVE_BUILDINGS];
    static float RemovedRadius[eMaxPool::MAX_REMOVE_BUILDINGS];
    static int TotalRemovedObjects;

public:
    static void Add(int modelID, CVector position, float radius);
    static void Delete();
    static void Process(int modelID, float posX, float posY, float posZ, float radius);
    static void RemoveOcclusionsInRadius(float posX, float posY, float posZ, float radius);
    static void MoveEntityOutOfView(CEntityGta* pEntity);
    static bool IsNeedRemoveObject(int modelID, float posX, float posY, float posZ);
};