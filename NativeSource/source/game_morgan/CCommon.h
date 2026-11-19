#pragma once

#include "Entity/CEntity.h"
#include "Entity/Ped/CPedGta.h"
#include "Models/CBaseModelInfo.h"

#include <unordered_map>

class CRect;
struct RwCamera;
struct RpLight;
struct RwTexture;

class CCommon
{
public:
	static uintptr_t GetModelInfoByID(int iModelID);
	static bool IsGameEntityArePlaceable(CEntityGta *pEntity);
    static bool IsGameEntityRenderable(CEntityGta *pEntity);
    static bool IsValidGamePed(CPedGta *pPed);
    static int GetTaskTypeFromTask(void* task);
    static bool IsPedModel(unsigned int iModelID);
    static bool IsExistForModelInfo(int iModelID);
    static bool ModelInfoGetCol(int iModel, const char* opcode);

    static void RwMatrixRotate(CMatrix* mat, int axis, float angle);
    static bool rpWorldAddCamera(RwCamera* camera);
    static void rpWorldRemoveCamera(RwCamera* camera);
    static void SetRenderWareCamera(RwCamera* camera);
    static RwCamera* GetRenderWareCamera();
    static bool rpWorldAddLight(RpLight* light);
    static void rpWorldRemoveLight(RpLight* light);
    static void DefinedState();
    static uint16_t ModelInfoGetReferenceCount(int iModelID);

    static std::unordered_map<int, bool> bTextDrawTextureSlotState;
    static std::unordered_map<int, RwTexture*> pTextDrawTexture;
    static bool IsPointInRect(float x, float y, CRect* rect);
    static int GetFreeTextDrawTextureSlot();
    static void DestroyTextDrawTexture(int index);
    static void DrawTextureUV(RwTexture* texture, CRect* rect, uint32_t dwColor, float* uv);
};