#pragma once

#include <cstdint>
#include <cstdio>

struct CdDirectoryEntry
{
    std::uint32_t sector;
    std::uint16_t sectors;
    std::uint16_t streamingSectors;
    char name[24];
};
static_assert(sizeof(CdDirectoryEntry) == 32);

class CdStreamInfo
{
public:
    static bool IsAvailable();
    static int CdStreamOpen(const char* fileName, bool isPlayerIMG, bool validateDirectory, std::uint32_t& largestEntry);

private:
    static bool ValidateDirectory(std::FILE* file, std::uint32_t& largestEntry);
};
