#include "../../Main.h"
#include "CdStreamInfo.h"
#include "Enums/eMaxArchives.h"

#include <mach-o/dyld.h>
#include <sys/stat.h>

bool CdStreamInfo::IsAvailable()
{
    return CMemory::HasMappedInstruction(SA_ADDR(0x219F9C), "\x88\x69\xF8\x97") && CMemory::HasMappedInstruction(SA_ADDR(0x034754), "\x16\x00\x00\xF9");
}

int CdStreamInfo::CdStreamOpen(const char* fileName, bool isPlayerIMG)
{
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    auto** handles = reinterpret_cast<void**>(SA_ADDR(0x509500) + slide);
    int slot = 0;

    while (slot < eMaxArchives::MAX_CD_HANDLE_ARCHIVES && handles[slot])
    {
        ++slot;
    }

    if (slot == eMaxArchives::MAX_CD_HANDLE_ARCHIVES)
    {
        CNecessary::Log(OBFUSCATE("IMG Out of Space"));
        return -1;
    }

    const auto open = reinterpret_cast<std::uint32_t (*)(const char*, bool)>(SA_ADDR(0x219CEC) + slide);
    const auto stream = open(fileName, isPlayerIMG);
    if (!handles[slot] || stream != (static_cast<std::uint32_t>(slot) << 24))
    {
        CNecessary::Log(OBFUSCATE("IMG open failed: %s"), fileName);
        return -1;
    }

    return static_cast<int>(stream);
}
