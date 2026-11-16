#pragma once

class CWeapon
{
public:
    static int GameGetWeaponModelIDFromWeaponID(int iWeaponID);
    static uintptr_t GetWeaponInfo(int iWeapon, int iSkill);
};