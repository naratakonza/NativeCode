#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

constexpr std::uint64_t SA_ADDR(std::uint64_t offset) noexcept
{
    return 0x100000000ULL + offset;
}

class CMemory
{
public:
    static void makeRET(std::uint64_t address);
    static void makeNOP(std::uint64_t address, std::size_t count);
    static void WriteProcessMemory(std::uint64_t address, std::uintptr_t bytes, std::size_t size);

    static constexpr std::array<std::uint8_t, 16> gtaUUID{
        0x0e, 0x42, 0xe8, 0xf2, 0x69, 0x92, 0x3e, 0xe9, 0xa7, 0x45, 0xb3, 0xfd, 0x65, 0x69, 0xa9, 0xfd
    };

    static constexpr std::size_t instructionSize = sizeof(std::uint32_t);

    static bool HasMappedInstruction(std::uint64_t address, const void* expected, const char* sectionName = "__text");

private:
    static bool HasMappedBytes(std::uint64_t address, const void* expected, std::size_t size, const char* sectionName);
};