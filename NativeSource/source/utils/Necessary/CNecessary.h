#pragma once

#include "../../vendor/obfuscate/str_obfuscate.hpp"

class CNecessary
{
public:
    static void Log(const char* fmt, ...);
    static std::uint32_t GetTickCount();
    static std::size_t MPSTRLEN(const char* str);
    static bool IsValidUTF8(const char* str, std::uint32_t maxLen = 0);
    static void ConvertLocalizedEncodingToUTF8(char* utf8Output, const char* input, std::uint32_t maxLen);
};
