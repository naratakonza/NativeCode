#include "../../Main.h"
#include "CdStreamInfo.h"
#include "Enums/eMaxArchives.h"

#include <mach-o/dyld.h>
#include <sys/stat.h>

struct CdDirectoryEntry
{
    std::uint32_t sector;
    std::uint16_t sectors;
    std::uint16_t streamingSectors;
    char name[24];
};
static_assert(sizeof(CdDirectoryEntry) == 32);

bool CdStreamInfo::IsAvailable()
{
    return CMemory::HasMappedInstruction(SA_ADDR(0x219F9C), "\x88\x69\xF8\x97") && CMemory::HasMappedInstruction(SA_ADDR(0x034754), "\x16\x00\x00\xF9");
}

int CdStreamInfo::CdStreamOpen(const char* fileName, bool isPlayerIMG, bool validateDirectory, std::uint32_t& largestEntry)
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

    if (validateDirectory)
    {
        std::FILE* file = nullptr;
        std::memcpy(&file, handles[slot], sizeof(file));

        if (!file || !CdStreamInfo::ValidateDirectory(file, largestEntry) || ::fseeko(file, 0, SEEK_SET) != 0)
        {
            const auto close = reinterpret_cast<int (*)(void*)>(SA_ADDR(0x034874) + slide);
            auto* names = reinterpret_cast<char (*)[eMaxArchives::MAX_CD_STREAM_IMAGE_NAME_SIZE]>(SA_ADDR(0x7AE520) + slide);
            close(handles[slot]);
            handles[slot] = nullptr;
            names[slot][0] = '\0';

            CNecessary::Log(OBFUSCATE("IMG invalid directory or sectors: %s"), fileName);
            return -1;
        }
    }

    return static_cast<int>(stream);
}

bool CdStreamInfo::ValidateDirectory(std::FILE* file, std::uint32_t& largestEntry)
{
    struct stat info{};
    if (::fstat(::fileno(file), &info) != 0 || info.st_size < 0) {
        return false;
    }

    std::uint32_t header[2]{};
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 0x32524556 || !header[1] || header[1] > INT32_MAX / sizeof(CdDirectoryEntry)) {
        return false;
    }

    constexpr std::uint64_t sectorSize = 2048;
    const auto directoryEnd = sizeof(header) + sizeof(CdDirectoryEntry) * static_cast<std::uint64_t>(header[1]);
    const auto fileSize = static_cast<std::uint64_t>(info.st_size);
    if (directoryEnd > fileSize) {
        return false;
    }

    std::uint32_t maxSectors = 0;
    for (std::uint32_t i = 0; i < header[1]; ++i)
    {
        CdDirectoryEntry entry{};
        if (std::fread(&entry, sizeof(entry), 1, file) != 1) {
            return false;
        }

        const auto start = entry.sector * sectorSize;
        const auto count = entry.streamingSectors ? entry.streamingSectors : entry.sectors;
        if (!count || start < directoryEnd || start > INT32_MAX || start + count * sectorSize > fileSize) {
            return false;
        }

        maxSectors = std::max(maxSectors, static_cast<std::uint32_t>(count));
    }

    largestEntry = maxSectors;
    return true;
}
