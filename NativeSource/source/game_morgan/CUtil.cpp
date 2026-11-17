#include "CUtil.h"
#include "CWorld.h"
#include "Core/CMatrix.h"
#include "../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>
#include <pthread.h>
#include <unordered_map>

CPedGta* CUtil::GamePool_FindPlayerPed()
{
    return CWorld::GetPlayerPed(0);
}

CPedGta* CUtil::GamePool_Ped_GetAt(int iID)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16DA74), "\xA0\x01\xF8\x37\x28\x2E\x00\xF0\x08\xED\x47\xF9", 12, "__text");
    if (!verified || !pthread_main_np()) {
        return nullptr;
    }

    return reinterpret_cast<CPedGta* (*)(int)>(SA_ADDR(0x16DA74) + _dyld_get_image_vmaddr_slide(0))(iID);
}

float CUtil::DegToRad(float fDegrees)
{
    if (fDegrees > 360.0f || fDegrees < 0.0f) return 0.0f;
    
    if (fDegrees > 180.0f) return (float)(-(PI - (((fDegrees - 180.0f) * PI) / 180.0f)));
    else return (float)((fDegrees * PI) / 180.0f);
}

float CUtil::GetDistanceBetween3DPoints(CVector* f, CVector* s)
{
	float dx = s->x - f->x;
	float dy = s->y - f->y;
	float dz = s->z - f->z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

float CUtil::GetDistanceBetweenPointAndLine(CVector* p, CVector* a, CVector* b)
{
    CVector pa = *p - *a;
    CVector pb = *p - *b;
    CVector ab = *b - *a;
    CVector cross;
    cross.x = pa.y * pb.z - pa.z * pb.y;
    cross.y = pa.z * pb.x - pa.x * pb.z;
    cross.z = pa.x * pb.y - pa.y * pb.x;
    float crossMag = sqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
    float lineMag = sqrt(ab.x * ab.x + ab.y * ab.y + ab.z * ab.z);

    if (lineMag < 0.0001f)
	{
		return CUtil::GetDistanceBetween3DPoints(p, a);
	}

    return crossMag / lineMag;
}

CVector CUtil::Multiply3x3(const CMatrix *m, const CVector *v)
{
    return reinterpret_cast<CVector (*)(const CMatrix *, const CVector *)>(SA_ADDR(0x22B874) + _dyld_get_image_vmaddr_slide(0))(m, v);
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