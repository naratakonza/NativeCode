#pragma once

#include <string>

class CMatrix;
class CVector;
struct CPedGta;

#define PI 3.14159265

class CUtil
{
public:
    static CPedGta* GamePool_FindPlayerPed();
    static CPedGta* GamePool_Ped_GetAt(int iID);
    static float DegToRad(float fDegrees);
    static float GetDistanceBetween3DPoints(CVector* f, CVector* s);
    static float GetDistanceBetweenPointAndLine(CVector* p, CVector* a, CVector* b);
    static CVector Multiply3x3(const CMatrix *m, const CVector *v);
    static std::string ReplaceKey(const std::string& input);
};