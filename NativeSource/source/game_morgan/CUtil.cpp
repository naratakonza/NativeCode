#include "CUtil.h"
#include "CWorld.h"
#include "../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>
#include <pthread.h>
#include <unordered_map>

CPedGta* CUtil::GamePool_FindPlayerPed()
{
    if (!pthread_main_np()) {
        return nullptr;
    }

    static const bool redirected = CMemory::HasMappedBytes(SA_ADDR(0x190C1C), "\x2A\x19\x00\xB0\x4A\x61\x42\xF9\x0B\x3B\x80\x52", 12, "__text");
    if (redirected)
    {
        return CWorld::Players[0].m_pPed;
    }

    static const bool stock = CMemory::HasMappedBytes(SA_ADDR(0x190C1C), "\x8A\x2D\x00\xB0\x4A\xA1\x29\x91\x0B\x3B\x80\x52\x29\x29\xAB\x9B", 16, "__text");
    return stock ? *reinterpret_cast<CPedGta* const*>(SA_ADDR(0x741A68) + _dyld_get_image_vmaddr_slide(0)) : nullptr;
}

CPedGta* CUtil::GamePool_Ped_GetAt(int iID)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16DA74), "\xA0\x01\xF8\x37\x28\x2E\x00\xF0\x08\xED\x47\xF9", 12, "__text");
    if (!verified || !pthread_main_np()) {
        return nullptr;
    }

    return reinterpret_cast<CPedGta* (*)(int)>(SA_ADDR(0x16DA74) + _dyld_get_image_vmaddr_slide(0))(iID);
}

std::string CUtil::ReplaceKey(const std::string& input)
{
    static const std::unordered_map<std::string, std::string> keyMap = {
        {"~k~~PED_ANSWER_PHONE~", "TAB"},
        {"~k~~VEHICLE_FIREWEAPON_ALT~", "ALT GR / LCTRL / NUM0"},
        {"~k~~PED_DUCK~", "C"},
        {"~k~~VEHICLE_HORN~", "H / CAPSLOCK"},
        {"~k~~PED_FIREWEAPON~", "LCTRL / LMB"},
        {"~k~~VEHICLE_FIREWEAPON~", "LALT"},
        {"~k~~PED_SPRINT~", "SPC"},
        {"~k~~VEHICLE_ACCELERATE~", "W"},
        {"~k~~VEHICLE_ENTER_EXIT~", "ENTER"},
        {"~k~~PED_JUMPING~", "LSHIFT"},
        {"~k~~VEHICLE_BRAKE~", "S"},
        {"~k~~VEHICLE_LOOKRIGHT~", "E"},
        {"~k~~PED_LOCK_TARGET~", "RMB"},
        {"~k~~VEHICLE_HANDBRAKE~", "SPC"},
        {"~k~~VEHICLE_LOOKLEFT~", "Q"},
        {"~k~~PED_LOOKBEHIND~", "NUM1 / MMB"},
        {"~k~~VEHICLE_LOOKBEHIND~", "2"},
        {"~k~~TOGGLE_SUBMISSIONS~", "2"},
        {"~k~~SNEAK_ABOUT~", "LALT"},
        {"~k~~VEHICLE_TURRETUP~", "NUM8"},
        {"~k~~VEHICLE_TURRETDOWN~", "NUM2"},
        {"~k~~VEHICLE_TURRETLEFT~", "NUM4"},
        {"~k~~VEHICLE_TURRETRIGHT~", "NUM6"},
        {"~k~~CONVERSATION_YES~", "Y"},
        {"~k~~CONVERSATION_NO~", "N"},
        {"~k~~GROUP_CONTROL_BWD~", "H"},
        {"~k~~GROUP_CONTROL_FWD~", "G"},
        {"~k~~GO_FORWARD~", "UP"},
        {"~k~~VEHICLE_STEERUP~", "UP"},
        {"~k~~GO_BACK~", "DOWN"},
        {"~k~~VEHICLE_STEERDOWN~", "DOWN"},
        {"~k~~GO_LEFT~", "LEFT"},
        {"~k~~VEHICLE_STEERLEFT~", "LEFT"},
        {"~k~~GO_RIGHT~", "RIGHT"},
        {"~k~~VEHICLE_STEERRIGHT~", "RIGHT"}
    };

    std::string output = input;

    for (const auto& pair : keyMap)
    {
        std::string::size_type pos = 0;
        while ((pos = output.find(pair.first, pos)) != std::string::npos)
        {
            output.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }

    return output;
}
