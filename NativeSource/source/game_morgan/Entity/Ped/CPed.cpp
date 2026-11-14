#include "CPed.h"
#include "../../Enums/eWeaponType.h"
#include "../../../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>

void CPed::GetBonePosition(CPedGta *thiz, CVector* posn, int boneTag, int bCalledFromCamera)
{
    if (!thiz || !pthread_main_np()) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x242998), "\x16\x10\x16\x91\xC8\x06\x40\xF9\x03\x01\x00\x34\x88\x02\x50\x37", 16, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(CPedGta*, CVector*, unsigned int, bool)>(SA_ADDR(0x24297C) + _dyld_get_image_vmaddr_slide(0))(thiz, posn, static_cast<unsigned int>(boneTag), bCalledFromCamera != 0);
    }
}

int CPed::GiveWeapon(CPedGta *thiz, int weaponType, int ammoQuantity, bool /*GenerateOldWeaponPickup*/)
{
    if (!thiz || !pthread_main_np()) {
        return 0;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x23C678), "\xE0\x03\x01\xAA\x21\x00\x80\x52\xE8\xF7\x07\x94\x13\x14\x40\xB9", 16, "__text");
    if (!verified) {
        return 0;
    }

    return reinterpret_cast<int (*)(CPedGta*, int, unsigned int)>(SA_ADDR(0x23C654) + _dyld_get_image_vmaddr_slide(0))(thiz, weaponType, static_cast<unsigned int>(ammoQuantity));
}

void CPed::SetCurrentWeapon(CPedGta *thiz, int weaponType)
{
    if (!thiz || !pthread_main_np()) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2432F8), "\xE0\x03\x01\xAA\x21\x00\x80\x52\xC8\xDC\x07\x94\x01\x14\x40\xB9", 16, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(CPedGta*, int)>(SA_ADDR(0x2432E8) + _dyld_get_image_vmaddr_slide(0))(thiz, weaponType);
    }
}

void CPed::ClearWeapons(CPedGta *thiz)
{
    if (!thiz || !pthread_main_np()) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x23CA2C), "\x01\x00\x80\x12\xDD\x18\x00\x94\xE0\x03\x13\xAA\x10\x19\x00\x94", 16, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(CPedGta*)>(SA_ADDR(0x23CA1C) + _dyld_get_image_vmaddr_slide(0))(thiz);
    }
}