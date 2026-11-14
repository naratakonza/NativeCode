#pragma once

#include <string>

struct CPedGta;

class CUtil
{
public:
    static CPedGta* GamePool_FindPlayerPed();
    static CPedGta* GamePool_Ped_GetAt(int iID);
    static std::string ReplaceKey(const std::string& input);
};