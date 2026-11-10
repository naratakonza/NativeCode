#pragma once

#include <cstdint>

#include "../../vendor/obfuscate/str_obfuscate.hpp"

class CNecessary
{
public:
    static void Log(const char* fmt, ...);
    static std::uint32_t GetTickCount();
};