#pragma once

#include "../Enums/eHookType.h"

#include <cstddef>
#include <cstdint>

struct TextureLoadBuffer
{
    std::uint8_t metadata[8];
    std::uint8_t* pixels;
    std::int32_t size;
    std::uint8_t ownsPixels;
};
static_assert(offsetof(TextureLoadBuffer, pixels) == 0x08);
static_assert(offsetof(TextureLoadBuffer, size) == 0x10);
static_assert(offsetof(TextureLoadBuffer, ownsPixels) == 0x14);

class CHooks
{
public:
    static void SetHooking(eHookType type);
    static void InstallNecessary();
    static void InstallMenu();
};
