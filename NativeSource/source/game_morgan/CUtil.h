#pragma once

#include <string>

class CMatrix;
class CVector;
struct CPedGta;

class CUtil
{
public:
    static CPedGta* GamePool_FindPlayerPed();
    static CPedGta* GamePool_Ped_GetAt(int iID);
    static CVector Multiply3x3(const CMatrix *m, const CVector *v);
    static std::string ReplaceKey(const std::string& input);
};