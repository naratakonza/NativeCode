#include "../../Main.h"
#include "CSprite2d.h"

#include <mach-o/dyld.h>

void CSprite2d::Constructor(RwTexture* pTexture)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x302624), "\x1F\x00\x00\xF9\xC0\x03\x5F\xD6", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(RwTexture**)>(SA_ADDR(0x302624) + _dyld_get_image_vmaddr_slide(0))(&pTexture);
    }
}

void CSprite2d::Destructor(RwTexture* pTexture)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x30262C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(RwTexture**)>(SA_ADDR(0x30262C) + _dyld_get_image_vmaddr_slide(0))(&pTexture);
    }
}

void CSprite2d::InitPerFrame()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x302884), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\x28\x2B\x00\xB0\x1F\x09\x06\xB9", 16, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)()>(SA_ADDR(0x302884) + _dyld_get_image_vmaddr_slide(0))();
    }
}

void CSprite2d::Draw(RwTexture* pTexture, CRect* rect, uint32_t dwColor, float* uv)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x302EA8), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\x28\x2B\x00\xB0\x10\x05\x46\xBD", 16, "__text");
    if (verified && rect && uv)
    {
        reinterpret_cast<void (*)(RwTexture**, CRect*, uint32_t*, float, float, float, float, float, float, float, float)>(SA_ADDR(0x302EA8) + _dyld_get_image_vmaddr_slide(0))(&pTexture, rect, &dwColor, uv[0], uv[1], uv[2], uv[3], uv[4], uv[5], uv[6], uv[7]);
    }
}

void CSprite2d::Draw(RwTexture* pTexture, CRect* rect, uint32_t dwColor)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x302CAC), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\x28\x2B\x00\xB0\x00\x05\x46\xBD", 16, "__text");
    if (verified && rect)
    {
        reinterpret_cast<void (*)(RwTexture**, CRect*, uint32_t*)>(SA_ADDR(0x302CAC) + _dyld_get_image_vmaddr_slide(0))(&pTexture, rect, &dwColor);
    }
}

RwTexture* CSprite2d::SetTexture(const char* szTexture)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x302690), "\xFF\x03\x02\xD1\xF8\x5F\x04\xA9\xF6\x57\x05\xA9", 12, "__text");
    RwTexture* pTexture = nullptr;
    
    if (verified)
    {
        reinterpret_cast<void (*)(RwTexture**, const char*)>(SA_ADDR(0x302690) + _dyld_get_image_vmaddr_slide(0))(&pTexture, szTexture);
    }

    return pTexture;
}

void CSprite2d::DrawBarChart(float x, float y, uint16_t w, uint8_t h, float value, uint8_t inrease, uint8_t legend, uint8_t border, CRGBA c, uint8_t* c2)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x303374), "\xFF\x43\x03\xD1\xEF\x3B\x04\x6D\xED\x33\x05\x6D", 12, "__text");
    if (verified && c2)
    {
        reinterpret_cast<void (*)(float, float, uint16_t, uint8_t, float, int8_t, uint8_t, uint8_t, const CRGBA*, const uint8_t*)>(SA_ADDR(0x303374) + _dyld_get_image_vmaddr_slide(0))(x, y, w, h, value, static_cast<int8_t>(inrease), legend, border, &c, c2);
    }
}