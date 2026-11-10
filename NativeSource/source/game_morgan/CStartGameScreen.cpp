#include "../../Main.h"
#include "CStartGameScreen.h"

#include <mach-o/dyld.h>

bool CStartGameScreen::OnNewGameCheck()
{
    const auto address = SA_ADDR(0x1EA2D8);
    if (!CMemory::HasMappedInstruction(address, "\x20\x00\x80\x52") || !CMemory::HasMappedInstruction(address + 4, "\x55\x3E\xF9\x17")) {
        CNecessary::Log(OBFUSCATE("CStartGameScreen OnNewGameCheck unavailable"));
        return false;
    }

    reinterpret_cast<void (*)()>(address + _dyld_get_image_vmaddr_slide(0))();
    return true;
}