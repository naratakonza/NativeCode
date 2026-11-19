#include "../../../Main.h"
#include "CMemory.h"

#include <cinttypes>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/vm_prot.h>
#include <stdint.h>

void CMemory::makeRET(std::uint64_t address)
{
    CMemory::WriteProcessMemory(address, (uintptr_t)"\xC0\x03\x5F\xD6", 4); // RET
}

void CMemory::makeNOP(std::uint64_t address, std::size_t count)
{
    if (!address || count > (UINT64_MAX - address) / instructionSize + 1) {
        return;
    }

    for (std::size_t index = 0; index < count; ++index)
    {
        CMemory::WriteProcessMemory(address + index * instructionSize, (uintptr_t)"\x1F\x20\x03\xD5", 4); // NOP
    }
}

void CMemory::WriteProcessMemory(std::uint64_t address, std::uintptr_t bytes, std::size_t size)
{
    const bool instruction = size == instructionSize;
    const auto* expected = reinterpret_cast<const void*>(bytes);

    if (instruction ? !HasMappedInstruction(address, expected) : !HasMappedBytes(address, expected, size, "__cstring"))
    {
        CNecessary::Log(OBFUSCATE("Patch bytes missing at 0x%" PRIX64), address);
    }
}

bool CMemory::InstallMethodHook(std::uint64_t address, std::uintptr_t method)
{
    if (method && address % sizeof(method) == 0 && HasMappedInstruction(SA_ADDR(0x23B32C), "\xF4\x4F\xBE\xA9"))
    {
        const auto* image = _dyld_get_image_header(0);
        const auto* cursor = reinterpret_cast<const std::uint8_t*>(image) + sizeof(mach_header_64);
        for (std::uint32_t i = 0; i < image->ncmds; ++i)
        {
            const auto* command = reinterpret_cast<const load_command*>(cursor);
            if (command->cmd == LC_SEGMENT_64)
            {
                const auto* segment = reinterpret_cast<const segment_command_64*>(command);
                if (std::strncmp(segment->segname, "__DATA_CONST", 16) == 0 && (segment->initprot & VM_PROT_READ))
                {
                    const auto* sections = reinterpret_cast<const section_64*>(segment + 1);
                    for (std::uint32_t j = 0; j < segment->nsects; ++j)
                    {
                        const auto& section = sections[j];
                        if (std::strncmp(section.sectname, "__const", 16) == 0 && std::strncmp(section.segname, "__DATA_CONST", 16) == 0 && address >= section.addr && section.size >= sizeof(method) && address - section.addr <= section.size - sizeof(method))
                        {
                            const auto* installed = reinterpret_cast<const std::uintptr_t*>(address + _dyld_get_image_vmaddr_slide(0));
                            if (*installed == method)
                            {
                                return true;
                            }
                        }
                    }
                }
            }

            cursor += command->cmdsize;
        }
    }

    CNecessary::Log(OBFUSCATE("Method hook missing at 0x%" PRIX64), address);
    return false;
}

bool CMemory::HasMappedInstruction(std::uint64_t address, const void* expected, const char* sectionName)
{
    return address % instructionSize == 0 && HasMappedBytes(address, expected, instructionSize, sectionName);
}

bool CMemory::HasMappedBytes(std::uint64_t address, const void* expected, std::size_t size, const char* sectionName)
{
    if (!expected || size == 0) {
        return false;
    }

    const auto* image = _dyld_get_image_header(0);
    if (!image || image->magic != MH_MAGIC_64 || image->cputype != CPU_TYPE_ARM64 || image->filetype != MH_EXECUTE || image->sizeofcmds > 1024 * 1024) {
        return false;
    }

    const auto* base = reinterpret_cast<const std::uint8_t*>(image);
    const auto* cursor = base + sizeof(mach_header_64);
    std::size_t remaining = image->sizeofcmds;
    const std::uint8_t* mapped = nullptr;
    bool verifiedUUID = false;

    for (std::uint32_t i = 0; i < image->ncmds; ++i)
    {
        if (remaining < sizeof(load_command)) {
            return false;
        }

        const auto* command = reinterpret_cast<const load_command*>(cursor);
        if (command->cmdsize < sizeof(*command) || command->cmdsize % 8 != 0 || command->cmdsize > remaining) {
            return false;
        }

        if (command->cmd == LC_UUID)
        {
            if (verifiedUUID || command->cmdsize != sizeof(uuid_command)) {
                return false;
            }

            const auto* identifier = reinterpret_cast<const uuid_command*>(command);
            if (std::memcmp(identifier->uuid, gtaUUID.data(), gtaUUID.size()) != 0) {
                return false;
            }

            verifiedUUID = true;
        }

        if (command->cmd == LC_SEGMENT_64)
        {
            if (command->cmdsize < sizeof(segment_command_64)) {
                return false;
            }

            const auto* segment = reinterpret_cast<const segment_command_64*>(command);
            if (segment->nsects > (command->cmdsize - sizeof(*segment)) / sizeof(section_64)) {
                return false;
            }

            if (segment->fileoff == 0 && std::strncmp(segment->segname, "__TEXT", 16) == 0 && (segment->initprot & (VM_PROT_READ | VM_PROT_EXECUTE)) == (VM_PROT_READ | VM_PROT_EXECUTE) && address >= segment->vmaddr)
            {
                const auto offset = address - segment->vmaddr;
                const auto* sections = reinterpret_cast<const section_64*>(segment + 1);

                for (std::uint32_t j = 0; j < segment->nsects; ++j)
                {
                    const auto& section = sections[j];
                    if (std::strncmp(section.sectname, sectionName, 16) != 0 || std::strncmp(section.segname, "__TEXT", 16) != 0 || address < section.addr || section.size < size || address - section.addr > section.size - size) {
                        continue;
                    }

                    if (mapped || offset < sizeof(mach_header_64) + image->sizeofcmds || segment->vmsize < size || offset > segment->vmsize - size || segment->filesize < size || offset > segment->filesize - size || offset != section.offset + (address - section.addr)) {
                        return false;
                    }

                    // dyld has already slid the executable header. __TEXT starts at file offset zero.
                    mapped = base + offset;
                }
            }
        }

        cursor += command->cmdsize;
        remaining -= command->cmdsize;
    }

    return verifiedUUID && remaining == 0 && mapped && std::memcmp(mapped, expected, size) == 0;
}