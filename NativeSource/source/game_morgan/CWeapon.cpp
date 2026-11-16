#include "../../Main.h"
#include "CWeapon.h"
#include "Enums/eWeaponSampid.h"
#include "Enums/eWeaponModel.h"

#include <mach-o/dyld.h>

uintptr_t CWeapon::GetWeaponInfo(int iWeapon, int iSkill)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x43A620), "\x3F\x0C\x00\x71\xE8\x01\x00\x54\xE8\x03\x01\x2A\x89\x01\x00\xD0", 16, "__text");
    if (!verified) {
        return 0;
    }

    return reinterpret_cast<uintptr_t (*)(int, int)>(SA_ADDR(0x43A620) + _dyld_get_image_vmaddr_slide(0))(iWeapon, iSkill);
}

int CWeapon::GameGetWeaponModelIDFromWeaponID(int iWeaponID)
{
	switch (iWeaponID)
	{
        case eWeaponSampId::WEAPON_BRASSKNUCKLE: {
            return eWeaponModel::WEAPON_MODEL_BRASSKNUCKLE;
        }

        case eWeaponSampId::WEAPON_GOLFCLUB: {
            return eWeaponModel::WEAPON_MODEL_GOLFCLUB;
        }

        case eWeaponSampId::WEAPON_NITESTICK: {
            return eWeaponModel::WEAPON_MODEL_NITESTICK;
        }

        case eWeaponSampId::WEAPON_KNIFE: {
            return eWeaponModel::WEAPON_MODEL_KNIFE;
        }

        case eWeaponSampId::WEAPON_BAT: {
            return eWeaponModel::WEAPON_MODEL_BAT;
        }

        case eWeaponSampId::WEAPON_SHOVEL: {
            return eWeaponModel::WEAPON_MODEL_SHOVEL;
        }

        case eWeaponSampId::WEAPON_POOLSTICK: {
            return eWeaponModel::WEAPON_MODEL_POOLSTICK;
        }

        case eWeaponSampId::WEAPON_KATANA: {
            return eWeaponModel::WEAPON_MODEL_KATANA;
        }

        case eWeaponSampId::WEAPON_CHAINSAW: {
            return eWeaponModel::WEAPON_MODEL_CHAINSAW;
        }

        case eWeaponSampId::WEAPON_DILDO: {
            return eWeaponModel::WEAPON_MODEL_DILDO;
        }

        case eWeaponSampId::WEAPON_DILDO2: {
            return eWeaponModel::WEAPON_MODEL_DILDO2;
        }

        case eWeaponSampId::WEAPON_VIBRATOR: {
            return eWeaponModel::WEAPON_MODEL_VIBRATOR;
        }

        case eWeaponSampId::WEAPON_VIBRATOR2: {
            return eWeaponModel::WEAPON_MODEL_VIBRATOR2;
        }

        case eWeaponSampId::WEAPON_FLOWER: {
            return eWeaponModel::WEAPON_MODEL_FLOWER;
        }

        case eWeaponSampId::WEAPON_CANE: {
            return eWeaponModel::WEAPON_MODEL_CANE;
        }

        case eWeaponSampId::WEAPON_GRENADE: {
            return eWeaponModel::WEAPON_MODEL_GRENADE;
        }

        case eWeaponSampId::WEAPON_TEARGAS: {
            return eWeaponModel::WEAPON_MODEL_TEARGAS;
        }

        case eWeaponSampId::WEAPON_MOLTOV: {
            return -1;
        }

        case eWeaponSampId::WEAPON_COLT45: {
            return eWeaponModel::WEAPON_MODEL_COLT45;
        }

        case eWeaponSampId::WEAPON_SILENCED: {
            return eWeaponModel::WEAPON_MODEL_SILENCED;
        }

        case eWeaponSampId::WEAPON_DEAGLE: {
            return eWeaponModel::WEAPON_MODEL_DEAGLE;
        }

        case eWeaponSampId::WEAPON_SHOTGUN: {
            return eWeaponModel::WEAPON_MODEL_SHOTGUN;
        }

        case eWeaponSampId::WEAPON_SAWEDOFF: {
            return eWeaponModel::WEAPON_MODEL_SAWEDOFF;
        }

        case eWeaponSampId::WEAPON_SHOTGSPA: {
            return eWeaponModel::WEAPON_MODEL_SHOTGSPA;
        }

        case eWeaponSampId::WEAPON_UZI: {
            return eWeaponModel::WEAPON_MODEL_UZI;
        }

        case eWeaponSampId::WEAPON_MP5: {
            return eWeaponModel::WEAPON_MODEL_MP5;
        }

        case eWeaponSampId::WEAPON_AK47: {
            return eWeaponModel::WEAPON_MODEL_AK47;
        }

        case eWeaponSampId::WEAPON_M4: {
            return eWeaponModel::WEAPON_MODEL_M4;
        }

        case eWeaponSampId::WEAPON_TEC9: {
            return eWeaponModel::WEAPON_MODEL_TEC9;
        }

        case eWeaponSampId::WEAPON_RIFLE: {
            return eWeaponModel::WEAPON_MODEL_RIFLE;
        }

        case eWeaponSampId::WEAPON_SNIPER: {
            return eWeaponModel::WEAPON_MODEL_SNIPER;
        }

        case eWeaponSampId::WEAPON_ROCKETLAUNCHER: {
            return eWeaponModel::WEAPON_MODEL_ROCKETLAUNCHER;
        }

        case eWeaponSampId::WEAPON_HEATSEEKER: {
            return eWeaponModel::WEAPON_MODEL_HEATSEEKER;
        }

        case eWeaponSampId::WEAPON_FLAMETHROWER: {
            return eWeaponModel::WEAPON_MODEL_FLAMETHROWER;
        }

        case eWeaponSampId::WEAPON_MINIGUN: {
            return eWeaponModel::WEAPON_MODEL_MINIGUN;
        }

        case eWeaponSampId::WEAPON_SATCHEL: {
            return eWeaponModel::WEAPON_MODEL_SATCHEL;
        }

        case eWeaponSampId::WEAPON_BOMB: {
            return eWeaponModel::WEAPON_MODEL_BOMB;
        }

        case eWeaponSampId::WEAPON_SPRAYCAN: {
            return eWeaponModel::WEAPON_MODEL_SPRAYCAN;
        }

        case eWeaponSampId::WEAPON_FIREEXTINGUISHER: {
            return eWeaponModel::WEAPON_MODEL_FIREEXTINGUISHER;
        }

        case eWeaponSampId::WEAPON_CAMERA: {
            return eWeaponModel::WEAPON_MODEL_CAMERA;
        }

        case -1: {
            return eWeaponModel::WEAPON_MODEL_NIGHTVISION;
        }

        case -2: {
            return eWeaponModel::WEAPON_MODEL_INFRARED;
        }

        case eWeaponSampId::WEAPON_PARACHUTE: {
            return eWeaponModel::WEAPON_MODEL_PARACHUTE;
        }

	}

	return -1;
}