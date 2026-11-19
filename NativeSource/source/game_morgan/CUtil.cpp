#include "CUtil.h"
#include "CFont.h"
#include "CWorld.h"
#include "Core/CMatrix.h"
#include "../utils/Memory/CMemory.h"
#include "../utils/Necessary/CNecessary.h"

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