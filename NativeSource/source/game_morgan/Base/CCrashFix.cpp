#include "../../../Main.h"
#include "CCrashFix.h"

extern "C" __attribute__((visibility("default"))) RwBool RpMaterialDestroy_hook(RpMaterial* material, RwBool (*original)(RpMaterial*))
{
    if (CNecessary::IsBadWritePtr(material, sizeof(*material)))
    {
        CNecessary::Log(OBFUSCATE("RpMaterialDestroy: material is nullptr or invalid/read-only!"));
        return 1;
    }

    return original(material);
}

extern "C" __attribute__((visibility("default"))) RwBool RwTextureDestroy_hook(RwTexture* texture, RwBool (*original)(RwTexture*))
{
    if (CNecessary::IsBadWritePtr(texture, sizeof(*texture)))
    {
        CNecessary::Log(OBFUSCATE("RwTextureDestroy: texture is nullptr or invalid!"));
        return 1;
    }

    // เดียวมาต่อภายหลัง
    // if (CGui::GetLicensePlate() && CGui::GetLicensePlate()->m_pDefaultPlate && texture == CGui::GetLicensePlate()->m_pDefaultPlate) {
    //     return 1;
    // }

    // if (texture->raster && !CNecessary::IsBadReadPtr(texture->raster))
    // {
    //     if (CLicensePlate::IsActiveLicensePlateTexture(texture)) {
    //         return 1;
    //     }
    // }

    return original(texture);
}

void CCrashFix::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCrashFix Initialise"));
}