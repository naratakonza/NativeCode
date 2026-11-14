#pragma once

#include "CPedGta.h"

class CPed
{
public:
	static void GetBonePosition(CPedGta *thiz, CVector* posn, int boneTag, int bCalledFromCamera);
    static int GiveWeapon(CPedGta *thiz, int weaponType, int ammoQuantity, bool GenerateOldWeaponPickup);
    static void SetCurrentWeapon(CPedGta *thiz, int weaponType);
    static void ClearWeapons(CPedGta *thiz);
};