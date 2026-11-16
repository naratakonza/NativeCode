#pragma once

#include "../../Enums/eMessageStyle.h"

class CGameText
{
public:
    static void Print(char* szStr, int iTime, eMessageStyle iType);

private:
    static uint16_t wszGameTextGXT[0xFF];
};