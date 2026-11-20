#pragma once

#include <cstdint>
#include <string>

class CMatrix;
class CVector;
struct CPedGta;
struct CPhysicalGta;

#define PI 3.14159265

class CUtil
{
public:
    static bool LIB_PointerGetCoordinates(int device, int* x, int* y, float* pressure);
    static CPedGta* GamePool_FindPlayerPed();
    static int GamePedStatPrim(int model_id);
    static CPedGta* GamePool_Ped_GetAt(int iID);
    static CPhysicalGta* GamePool_Object_GetAt(int iID);
    static void RpAnimBlendClumpUpdateAnimations(uintptr_t pClump, float fTimeStep, int bOnScreen);
    static float DegToRad(float fDegrees);
    static float FloatOffset(float f1, float f2);
    static float GetDistance(CVector vec1, CVector vec2);
    static bool IsPedPointerValid(uintptr_t pPed);
    static uintptr_t FindFistTarget(CPedGta* pPlayerPed);
    static float GetDistanceBetween3DPoints(CVector* f, CVector* s);
    static float GetDistanceBetweenPointAndLine(CVector* p, CVector* a, CVector* b);
    static CVector Multiply3x3(const CMatrix *m, const CVector *v);
    static void FilterColors(char* szStr);
    static std::string ReplaceKey(const std::string& input);
    static std::string ProcessTextDrawString(const std::string& input);
    static bool HasNewlineTag(const std::string& str);
    static void DrawMultiLineTextDraw(float fUseX, float fUseY, const std::string& strString);
};