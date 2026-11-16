#pragma once

#include "../../Enums/eMessageStyle.h"

#include <cstdint>

class CGameText
{
public:
    static void Print(char* szStr, int iTime, eMessageStyle iType);

private:
    static uint16_t wszGameTextGXT[0xFF];
};
