#pragma once

#include "../Enums/eHookType.h"

class CHooks
{
public:
    static void SetHooking(eHookType type);
    static void InstallNecessary();
    static void InstallMenu();
};