#include "../../Main.h"
#include "CFont.h"

#include <cstring>
#include <mach-o/dyld.h>

uint16_t CFont::ms_text[0x800]{};

void CFont::SetWrapx(float fVal)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3754), "\x88\x2A\x00\x90\x00\xC5\x0F\xBD\xC0\x03\x5F\xD6", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(float)>(SA_ADDR(0x2F3754) + _dyld_get_image_vmaddr_slide(0))(fVal);
    }
}

void CFont::SetCentreSize(float fVal)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3760), "\x88\x2A\x00\x90\x00\xC9\x0F\xBD\xC0\x03\x5F\xD6", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(float)>(SA_ADDR(0x2F3760) + _dyld_get_image_vmaddr_slide(0))(fVal);
    }
}

void CFont::SetDropColor(CRGBA* dwColor)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3784), "\x88\x2A\x00\x90\x08\xF1\x3E\x91\x09\x00\x40\x39", 12, "__text");
    if (verified && dwColor)
    {
        reinterpret_cast<void (*)(const CRGBA*)>(SA_ADDR(0x2F3784) + _dyld_get_image_vmaddr_slide(0))(dwColor);
    }
}

void CFont::SetDropColor(uint32_t* dwColor)
{
    CFont::SetDropColor(reinterpret_cast<CRGBA*>(dwColor));
}

void CFont::SetDropShadowPosition(uint8_t iPos)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F0FD0), "\x88\x2A\x00\xF0\x08\x49\x3F\x91\x1F\x51\x00\x78", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t)>(SA_ADDR(0x2F0FD0) + _dyld_get_image_vmaddr_slide(0))(iPos);
    }
}

void CFont::SetEdge(uint8_t iPos)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F37F8), "\x28\x2B\x00\xD0\x08\xA1\x4F\x39\x1F\x01\x00\x71", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(int8_t)>(SA_ADDR(0x2F37F8) + _dyld_get_image_vmaddr_slide(0))(static_cast<int8_t>(iPos));
    }
}

void CFont::SetBackgroundColor(uint32_t* dwColor)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3844), "\x08\x00\x40\x39\x89\x2A\x00\x90\x29\x01\x3F\x91", 12, "__text");
    if (verified && dwColor)
    {
        reinterpret_cast<void (*)(const uint32_t*)>(SA_ADDR(0x2F3844) + _dyld_get_image_vmaddr_slide(0))(dwColor);
    }
}

void CFont::SetJustify(uint8_t iPos)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3870), "\x88\x2A\x00\x90\x00\xD1\x3E\x39\xC0\x03\x5F\xD6", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t)>(SA_ADDR(0x2F3870) + _dyld_get_image_vmaddr_slide(0))(iPos);
    }
}

void CFont::SetScale(float fScale)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3670), "\x28\xAE\x87\x52\x48\xE1\xA7\x72\x01\x01\x27\x1E", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(float)>(SA_ADDR(0x2F3670) + _dyld_get_image_vmaddr_slide(0))(fScale);
    }
}

void CFont::SetScale(float fScaleX, float fScaleY)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3680), "\x88\x2A\x00\x90\x08\x81\x3E\x91\x01\x01\x00\xBD\x00\x05\x00\xBD", 16, "__text");
    if (verified)
    {
        const auto slide = _dyld_get_image_vmaddr_slide(0);
        *reinterpret_cast<float*>(SA_ADDR(0x843FA0) + slide) = fScaleX;
        *reinterpret_cast<float*>(SA_ADDR(0x843FA4) + slide) = fScaleY;
    }
}

void CFont::SetBackground(uint8_t unk0, uint8_t unk1)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3830), "\x88\x2A\x00\x90\x08\xDD\x3E\x91\x00\x01\x00\x39", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t, uint8_t)>(SA_ADDR(0x2F3830) + _dyld_get_image_vmaddr_slide(0))(unk0, unk1);
    }
}

void CFont::SetProportional(uint8_t iProp)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3824), "\x88\x2A\x00\x90\x00\xE5\x3E\x39\xC0\x03\x5F\xD6", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t)>(SA_ADDR(0x2F3824) + _dyld_get_image_vmaddr_slide(0))(iProp);
    }
}

void CFont::SetOrientation(uint8_t iOrient)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F387C), "\x1F\x08\x00\x71\x48\x01\x00\x54\x08\x7C\x7D\xD3", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t)>(SA_ADDR(0x2F387C) + _dyld_get_image_vmaddr_slide(0))(iOrient);
    }
}

void CFont::SetColor(uint32_t* dwColor)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F3430), "\x89\x2A\x00\x90\x29\x71\x3E\x91\x08\x00\x40\x39", 12, "__text");
    if (verified && dwColor)
    {
        reinterpret_cast<void (*)(const uint32_t*)>(SA_ADDR(0x2F3430) + _dyld_get_image_vmaddr_slide(0))(dwColor);
    }
}

void CFont::SetFontStyle(uint8_t iStyle)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F36B4), "\x28\x2B\x00\xD0\x08\xA1\x4F\x39\x29\x2B\x00\xD0", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uint8_t)>(SA_ADDR(0x2F36B4) + _dyld_get_image_vmaddr_slide(0))(iStyle);
    }
}

void CFont::AsciiToGxtChar(const char* ascii, uint16_t* gxt)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F0310), "\x20\x01\x00\xB4\x09\x00\x40\x39\xC9\x00\x00\x34", 12, "__text");
    if (verified && ascii && gxt)
    {
        reinterpret_cast<void (*)(const char*, uint16_t*)>(SA_ADDR(0x2F0310) + _dyld_get_image_vmaddr_slide(0))(ascii, gxt);
    }
}

void CFont::RenderFontBuffer()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F15C8), "\xFF\x83\x02\xD1\xED\x33\x01\x6D\xEB\x2B\x02\x6D", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)()>(SA_ADDR(0x2F15C8) + _dyld_get_image_vmaddr_slide(0))();
    }
}

void CFont::PrintString(float posX, float posY, const char* string)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F235C), "\xFF\x83\x01\xD1\xE9\x23\x02\x6D\xF6\x57\x03\xA9", 12, "__text");
    if (!verified || !string) {
        return;
    }

    char ascii[sizeof(CFont::ms_text) / sizeof(CFont::ms_text[0])];
    const auto length = strnlen(string, sizeof(ascii) - 1);
    std::memcpy(ascii, string, length);
    ascii[length] = '\0';

    CFont::ms_text[0] = 0;
    CFont::AsciiToGxtChar(ascii, CFont::ms_text);
    reinterpret_cast<void (*)(float, float, const uint16_t*)>(SA_ADDR(0x2F235C) + _dyld_get_image_vmaddr_slide(0))(posX, posY, CFont::ms_text);
    CFont::RenderFontBuffer();
}

float CFont::GetHeight(bool unk)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F1590), "\x88\x2A\x00\xD0\x08\xB1\x3F\x91\x89\x2A\x00\xD0", 12, "__text");
    if (verified)
    {
        return reinterpret_cast<float (*)(bool)>(SA_ADDR(0x2F1590) + _dyld_get_image_vmaddr_slide(0))(unk);
    }

    return 0.0f;
}

float CFont::GetStringWidth(uint16_t* str)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F2E74), "\xEB\x2B\xB8\x6D\xE9\x23\x01\x6D\xFC\x6F\x02\xA9", 12, "__text");
    if (verified && str)
    {
        return reinterpret_cast<float (*)(const uint16_t*, bool, bool)>(SA_ADDR(0x2F2E74) + _dyld_get_image_vmaddr_slide(0))(str, true, false);
    }

    return 0.0f;
}

void CFont::GetTextRect(CRect* rect, float x, float y)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F252C), "\xE9\x23\xBD\x6D\xF4\x4F\x01\xA9\xFD\x7B\x02\xA9\xFD\x83\x00\x91\x28\x40\x20\x1E\x09\x40\x20\x1E\xF3\x03\x00\xAA\x00\x00\x80\x52\x39\x00\x00\x94", 36, "__text");
    if (verified && rect)
    {
        reinterpret_cast<void (*)(CRect*, float, float, const uint16_t*)>(SA_ADDR(0x2F252C) + _dyld_get_image_vmaddr_slide(0))(rect, x, y, CFont::ms_text);
    }
}