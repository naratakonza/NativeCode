#pragma once

#include <cstdint>
#include <cstdio>

class CdStreamInfo
{
public:
    static bool IsAvailable();
    static int CdStreamOpen(const char* fileName, bool isPlayerIMG, bool validateDirectory, std::uint32_t& largestEntry);

private:
    static bool ValidateDirectory(std::FILE* file, std::uint32_t& largestEntry);
};
