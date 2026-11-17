#include "../../../../Main.h"
#include "CGameText.h"
#include "../../CFont.h"
#include "../../Scripts/CScripting.h"

#include <mach-o/dyld.h>

uint16_t CGameText::wszGameTextGXT[0xFF]{};

void CGameText::Print(char* szStr, int iTime, eMessageStyle iType)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3D5B88), "\x08\x29\x00\x90\x08\x81\x19\x91\x09\x24\x80\x52\x48\x20\xA9\x9B", 16, "__text");
    if (!verified || !pthread_main_np() || !szStr) {
        return;
    }

    char ascii[sizeof(CGameText::wszGameTextGXT) / sizeof(CGameText::wszGameTextGXT[0])];
    const auto length = strnlen(szStr, sizeof(ascii) - 1);
    std::memcpy(ascii, szStr, length);
    ascii[length] = '\0';

    CScripting::ScriptCommand(&text_clear_all);
    CGameText::wszGameTextGXT[0] = 0;
    CFont::AsciiToGxtChar(ascii, CGameText::wszGameTextGXT);

    // CMessages::AddBigMessage
    reinterpret_cast<void (*)(const uint16_t*, int, unsigned int)>(SA_ADDR(0x3D5B88) + _dyld_get_image_vmaddr_slide(0))(CGameText::wszGameTextGXT, iTime, iType);
}