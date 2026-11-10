#pragma once

#include <cstddef>
#include <cstdint>

class CPools
{
public:
    std::uint8_t* objects;
    std::uint8_t* flags;
    int count;
    int top;
    bool bOwnsAllocations;
    bool m_bDealWithNoMemory;
    std::uint8_t padding[6];

    static void Install();
    static void Initialise(void (*original)());

private:
    static CPools* Allocate(std::size_t count, std::size_t stride);
};

static_assert(sizeof(CPools) == 0x20);
static_assert(offsetof(CPools, objects) == 0x00);
static_assert(offsetof(CPools, flags) == 0x08);
static_assert(offsetof(CPools, count) == 0x10);
static_assert(offsetof(CPools, top) == 0x14);
static_assert(offsetof(CPools, bOwnsAllocations) == 0x18);