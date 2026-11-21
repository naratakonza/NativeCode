#include "CUtil.h"
#include "CFont.h"
#include "Models/CModelInfo.h"
#include "CWorld.h"
#include "CPools.h"
#include "Entity/Ped/CPedGta.h"
#include "Core/CMatrix.h"
#include "../utils/Memory/CMemory.h"
#include "../utils/Necessary/CNecessary.h"

#include <cmath>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <unordered_map>
#include <vector>

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

CPhysicalGta* CUtil::GamePool_Object_GetAt(int iID)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16DB4C), "\xA0\x01\xF8\x37\x28\x2E\x00\xF0\x08\xF9\x47\xF9", 12, "__text");
    if (!verified || !pthread_main_np()) {
        return nullptr;
    }

    return reinterpret_cast<CPhysicalGta* (*)(int)>(SA_ADDR(0x16DB4C) + _dyld_get_image_vmaddr_slide(0))(iID);
}

void CUtil::RpAnimBlendClumpUpdateAnimations(uintptr_t pClump, float fTimeStep, int bOnScreen)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3FE98), "\xFF\x03\x04\xD1\xEB\x2B\x08\x6D", 8, "__text");
    if (verified && pClump && pthread_main_np())
    {
        reinterpret_cast<void (*)(uintptr_t, float, bool)>(SA_ADDR(0x3FE98) + _dyld_get_image_vmaddr_slide(0))(pClump, fTimeStep, bOnScreen != 0);
    }
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

std::string CUtil::ProcessTextDrawString(const std::string& input)
{
    if (input.empty()) {
        return "";
    }

    std::string output = "";
    bool pc_highlight = false;

    size_t i = 0;
    size_t len = input.length();

    while (i < len)
    {
        if (input[i] == '~' && i + 2 < len && input[i + 2] == '~')
        {
            char tag = input[i + 1];
            char lower_tag = (tag >= 'A' && tag <= 'Z') ? (tag - 'A' + 'a') : tag;

            if (lower_tag == 'h')
            {
                pc_highlight = !pc_highlight;
                output += std::string(OBFUSCATE("~h~"));
                i += 3;
            }
            else if (lower_tag == 'n')
            {
                output += std::string(OBFUSCATE("~n~~w~"));

                if (pc_highlight)
                {
                    output += std::string(OBFUSCATE("~h~"));
                }

                i += 3;
            }
            else if (lower_tag == 'r' || lower_tag == 'g' || lower_tag == 'b' || lower_tag == 'w' || lower_tag == 'y' || lower_tag == 'p' || lower_tag == 'l')
            {
                pc_highlight = false;

                if (i + 8 < len && input[i + 3] == '~' && (input[i + 4] == 'h' || input[i + 4] == 'H') && input[i + 5] == '~' && input[i + 6] == '~' && (input[i + 7] == 'h' || input[i + 7] == 'H') && input[i + 8] == '~')
                {
                    output += std::string(OBFUSCATE("~"));
                    output += tag;
                    output += std::string(OBFUSCATE("~~h~~h~"));

                    pc_highlight = true;
                    i += 9;
                }
                else if (i + 5 < len && input[i + 3] == '~' && (input[i + 4] == 'h' || input[i + 4] == 'H') && input[i + 5] == '~')
                {
                    output += std::string(OBFUSCATE("~"));
                    output += tag;
                    output += std::string(OBFUSCATE("~~h~~h~"));

                    pc_highlight = true;
                    i += 6;
                }
                else
                {
                    output += std::string(OBFUSCATE("~"));
                    output += tag;
                    output += std::string(OBFUSCATE("~"));
                    i += 3;
                }
            }
            else
            {
                if (lower_tag == 's')
                {
                    pc_highlight = false;
                }

                output += std::string(OBFUSCATE("~"));
                output += tag;
                output += std::string(OBFUSCATE("~"));
                i += 3;
            }
        }
        else
        {
            output += input[i];
            i++;
        }
    }

    return output;
}

bool CUtil::HasNewlineTag(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    size_t i = 0;
    size_t len = str.length();
    while (i < len)
    {
        if (str[i] == '~' && i + 2 < len && str[i + 2] == '~')
        {
            char tag = str[i + 1];
            char lower_tag = (tag >= 'A' && tag <= 'Z') ? (tag - 'A' + 'a') : tag;

            if (lower_tag == 'n')
            {
                return true;
            }
            else if (lower_tag == 'r' || lower_tag == 'g' || lower_tag == 'b' || lower_tag == 'w' || lower_tag == 'y' || lower_tag == 'p' || lower_tag == 'l')
            {
                if (i + 8 < len && str[i + 3] == '~' && (str[i + 4] == 'h' || str[i + 4] == 'H') && str[i + 5] == '~' && str[i + 6] == '~' && (str[i + 7] == 'h' || str[i + 7] == 'H') && str[i + 8] == '~')
                {
                    i += 9;
                    continue;
                }
                else if (i + 5 < len && str[i + 3] == '~' && (str[i + 4] == 'h' || str[i + 4] == 'H') && str[i + 5] == '~')
                {
                    i += 6;
                    continue;
                }
            }

            i += 3;
        }
        else
        {
            i++;
        }
    }

    return false;
}

void CUtil::DrawMultiLineTextDraw(float fUseX, float fUseY, const std::string& strString)
{
    std::vector<std::string> raw_lines;
    std::string temp = strString;

    while (true)
    {
        size_t p = std::string::npos;
        size_t delim_len = 3;

        size_t i = 0;
        size_t len = temp.length();
        while (i < len)
        {
            if (temp[i] == '~' && i + 2 < len && temp[i + 2] == '~')
            {
                char tag = temp[i + 1];
                char lower_tag = (tag >= 'A' && tag <= 'Z') ? (tag - 'A' + 'a') : tag;

                if (lower_tag == 'n')
                {
                    p = i;
                    delim_len = 3;
                    break;
                }
                else if (lower_tag == 'r' || lower_tag == 'g' || lower_tag == 'b' || lower_tag == 'w' || lower_tag == 'y' || lower_tag == 'p' || lower_tag == 'l')
                {
                    if (i + 8 < len && temp[i + 3] == '~' && (temp[i + 4] == 'h' || temp[i + 4] == 'H') && temp[i + 5] == '~' && temp[i + 6] == '~' && (temp[i + 7] == 'h' || temp[i + 7] == 'H') && temp[i + 8] == '~')
                    {
                        i += 9;
                        continue;
                    }
                    else if (i + 5 < len && temp[i + 3] == '~' && (temp[i + 4] == 'h' || temp[i + 4] == 'H') && temp[i + 5] == '~')
                    {
                        i += 6;
                        continue;
                    }
                }
                i += 3;
            }
            else
            {
                i++;
            }
        }

        if (p == std::string::npos) {
            raw_lines.push_back(temp);
            break;
        }

        raw_lines.push_back(temp.substr(0, p));
        temp = temp.substr(p + delim_len);
    }

    bool pc_highlight = false;
    float currentY = fUseY;
    float lineHeight = CFont::GetHeight(false);

    for (size_t k = 0; k < raw_lines.size(); k++)
    {
        std::string line = raw_lines[k];
        if (pc_highlight)
        {
            line = std::string(OBFUSCATE("~h~")) + line;
        }

        std::string processed_line = "";
        size_t j = 0;
        size_t len = line.length();
        while (j < len)
        {
            if (line[j] == '~' && j + 2 < len && line[j + 2] == '~')
            {
                char tag = line[j + 1];
                char lower_tag = (tag >= 'A' && tag <= 'Z') ? (tag - 'A' + 'a') : tag;

                if (lower_tag == 'h')
                {
                    pc_highlight = !pc_highlight;
                    processed_line += std::string(OBFUSCATE("~h~"));
                    j += 3;
                }
                else if (lower_tag == 'r' || lower_tag == 'g' || lower_tag == 'b' || lower_tag == 'w' || lower_tag == 'y' || lower_tag == 'p' || lower_tag == 'l')
                {
                    pc_highlight = false;

                    if (j + 8 < len && line[j + 3] == '~' && (line[j + 4] == 'h' || line[j + 4] == 'H') && line[j + 5] == '~' && line[j + 6] == '~' && (line[j + 7] == 'h' || line[j + 7] == 'H') && line[j + 8] == '~')
                    {
                        processed_line += std::string(OBFUSCATE("~"));
                        processed_line += tag;
                        processed_line += std::string(OBFUSCATE("~~h~~h~"));

                        pc_highlight = true;
                        j += 9;
                    }
                    else if (j + 5 < len && line[j + 3] == '~' && (line[j + 4] == 'h' || line[j + 4] == 'H') && line[j + 5] == '~')
                    {
                        processed_line += std::string(OBFUSCATE("~"));
                        processed_line += tag;
                        processed_line += std::string(OBFUSCATE("~~h~~h~"));

                        pc_highlight = true;
                        j += 6;
                    }
                    else
                    {
                        processed_line += std::string(OBFUSCATE("~"));
                        processed_line += tag;
                        processed_line += std::string(OBFUSCATE("~"));
                        j += 3;
                    }
                }
                else
                {
                    if (lower_tag == 's')
                    {
                        pc_highlight = false;
                    }

                    processed_line += std::string(OBFUSCATE("~"));
                    processed_line += tag;
                    processed_line += std::string(OBFUSCATE("~"));
                    j += 3;
                }
            }
            else
            {
                processed_line += line[j];
                j++;
            }
        }

        CFont::PrintString(fUseX, currentY, reinterpret_cast<const char*>(processed_line.c_str()));
        currentY += lineHeight;
    }
}

bool CUtil::LIB_PointerGetCoordinates(int device, int* x, int* y, float* pressure)
{
    return reinterpret_cast<bool (*)(int, int*, int*, float*)>(SA_ADDR(0x0254AC) + _dyld_get_image_vmaddr_slide(0))(device, x, y, pressure);
}

float CUtil::FloatOffset(float f1, float f2)
{
    if (f1 >= f2)
    {
        return f1 - f2;
    }
    else
    {
        return (f2 - f1);
    }
}

void CUtil::FilterColors(char* szStr)
{
    if (!szStr)
    {
        return;
    }

    char szNonColored[2048 + 1];
    int iNonColoredMsgLen = 0;

    for (std::size_t pos = 0; pos < CNecessary::MPSTRLEN(szStr) && szStr[pos] != '\0'; pos++)
    {
        if (pos+7 < CNecessary::MPSTRLEN(szStr))
        {
            if (szStr[pos] == '{' && szStr[pos+7] == '}')
            {
                pos += 7;
                continue;
            }
        }

        szNonColored[iNonColoredMsgLen] = szStr[pos];
        iNonColoredMsgLen++;
    }

    szNonColored[iNonColoredMsgLen] = 0;
    strcpy(szStr, szNonColored);
}

float CUtil::GetDistance(CVector vec1, CVector vec2)
{
    return sqrt((vec1.z - vec2.z) * (vec1.z - vec2.z) + (vec1.y - vec2.y) * (vec1.y - vec2.y) + (vec1.x - vec2.x) * (vec1.x - vec2.x));
}

bool CUtil::IsPedPointerValid(uintptr_t pPed)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16DA74), "\xA0\x01\xF8\x37\x28\x2E\x00\xF0\x08\xED\x47\xF9", 12, "__text");
    if (!verified || !pthread_main_np())
    {
        return false;
    }

    if (pPed == 0) {
        return false;
    }

    CPools* pPedPool = *reinterpret_cast<CPools**>(SA_ADDR(0x734FD8) + _dyld_get_image_vmaddr_slide(0));
    if (pPedPool == nullptr || pPedPool->objects == nullptr) {
        return false;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(pPedPool->objects);
    uintptr_t size = 2504;
    uintptr_t count = pPedPool->count;
    uintptr_t end = start + size * count;

    if (pPed >= start && pPed < end && ((pPed - start) % size) == 0)
    {
        size_t index = (pPed - start) / size;
        if (pPedPool->flags != nullptr)
        {
            return (pPedPool->flags[index] & 0x80) == 0;
        }
    }

    return false;
}

uintptr_t CUtil::FindFistTarget(CPedGta* pPlayerPed)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16DA74), "\xA0\x01\xF8\x37\x28\x2E\x00\xF0\x08\xED\x47\xF9", 12, "__text");
    if (!verified || !pthread_main_np())
    {
        return 0;
    }

    if (pPlayerPed == nullptr) {
        return 0;
    }

    CPools* pPedPool = *reinterpret_cast<CPools**>(SA_ADDR(0x734FD8) + _dyld_get_image_vmaddr_slide(0));
    if (pPedPool == nullptr || pPedPool->objects == nullptr) {
        return 0;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(pPedPool->objects);
    uintptr_t size = 2504;
    uintptr_t count = pPedPool->count;

    uintptr_t bestTarget = 0;
    float bestDist = 5.0f; // ระยะโจมตีสูงสุด 5 เมตรสำหรับหมัด

    CVector playerPos;
    if (pPlayerPed->mat != nullptr)
    {
        playerPos = pPlayerPed->mat->pos;
    }
    else
    {
        playerPos = pPlayerPed->m_transform.m_vPosn;
    }

    CVector vecForward;
    if (pPlayerPed->mat != nullptr)
    {
        vecForward = pPlayerPed->mat->up;
    }
    else
    {
        float heading = pPlayerPed->m_transform.m_fHeading;
        vecForward.x = -std::sin(heading);
        vecForward.y = std::cos(heading);
        vecForward.z = 0.0f;
    }

    float len2d = std::sqrt(vecForward.x * vecForward.x + vecForward.y * vecForward.y);
    if (len2d > 0.0f)
    {
        vecForward.x /= len2d;
        vecForward.y /= len2d;
    }

    for (size_t i = 0; i < count; ++i)
    {
        if ((pPedPool->flags[i] & 0x80) != 0) {
            continue;
        }

        uintptr_t pPedAddr = start + i * size;
        if (pPedAddr == reinterpret_cast<uintptr_t>(pPlayerPed)) {
            continue;
        }

        CPedGta* pPed = reinterpret_cast<CPedGta*>(pPedAddr);
        if (pPed->fHealth <= 0.1f) {
            continue;
        }

        CVector targetPos;
        if (pPed->mat != nullptr)
        {
            targetPos = pPed->mat->pos;
        }
        else
        {
            targetPos = pPed->m_transform.m_vPosn;
        }

        float dist = CUtil::GetDistance(playerPos, targetPos);
        if (dist <= 5.0f)
        {
            CVector dir;
            dir.x = targetPos.x - playerPos.x;
            dir.y = targetPos.y - playerPos.y;
            dir.z = 0.0f;

            float dirLen = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (dirLen > 0.0f)
            {
                dir.x /= dirLen;
                dir.y /= dirLen;
            }

            float dot = dir.x * vecForward.x + dir.y * vecForward.y;
            if (dot > 0.5f)
            {
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestTarget = pPedAddr;
                }
            }
        }
    }

    return bestTarget;
}

int CUtil::GamePedStatPrim(int model_id)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x23CE1C), "\x88\x5A\x80\xB9") && CMemory::HasMappedInstruction(SA_ADDR(0x23CE34), "\x68\x96\x03\xF9");
    if (!verified) {
        return -1;
    }

    CBaseModelInfo* pModelInfo = CModelInfo::GetModelInfo(model_id);
    return pModelInfo ? static_cast<int>(pModelInfo->AsPedModelInfoPtr()->m_defaultPedStats) : -1;
}

void CUtil::CalculateAimVector(CVector* vec1, CVector* vec2)
{
    float f1;
    float f2;
    float f3;

    f1 = atan2(vec1->x, vec1->y) - 1.570796370506287;
    f2 = sin(f1);
    f3 = cos(f1);
    vec2->x = vec1->y * 0.0 - f3 * vec1->z;
    vec2->y = f2 * vec1->z - vec1->x * 0.0;
    vec2->z = f3 * vec1->x - f2 * vec1->y;
}