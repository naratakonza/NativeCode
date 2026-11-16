#pragma once

#include <cstdint>

using HSTREAM = std::uint32_t;
using HRECORD = std::uint32_t;
using HFX = std::uint32_t;
using HSYNC = std::uint32_t;

struct BASS_3DVECTOR
{
    float x;
    float y;
    float z;
};

struct BASS_DEVICEINFO
{
    const char* name;
    const char* driver;
    std::uint32_t flags;
};

using STREAMPROC = std::uint32_t(HSTREAM, void*, std::uint32_t, void*);
using DOWNLOADPROC = void(const void*, std::uint32_t, void*);
using RECORDPROC = int(HRECORD, const void*, std::uint32_t, void*);
using SYNCPROC = void(HSYNC, std::uint32_t, std::uint32_t, void*);

#define MORGAN_BASS_API(result, name, args) extern result (*name) args;
#include "CBassApi.inc"
#undef MORGAN_BASS_API

class CBass
{
public:
    static bool Initialise();
};