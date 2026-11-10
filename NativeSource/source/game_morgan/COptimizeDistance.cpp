#include "../../Main.h"
#include "COptimizeDistance.h"

#include <mach-o/dyld.h>

void COptimizeDistance::OptimizeStreamingMemory(std::uint32_t memoryInBytes)
{
    const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x30939C), "\xF4\x0E\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x3093A4), "\x89\xCE\x4C\xB9") && CMemory::HasMappedInstruction(SA_ADDR(0x3093A8), "\x29\x01\x13\x4B");
    if (!verified)
    {
        CNecessary::Log(OBFUSCATE("COptimizeDistance OptimizeStreamingMemory: GTA iOS layout mismatch"));
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    auto& available = *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x4E8CCC) + slide); // CStreaming::ms_memoryAvailable
    const auto previous = available;
    available = std::clamp(memoryInBytes, 128U * 1024 * 1024, 512U * 1024 * 1024);
    CNecessary::Log(OBFUSCATE("COptimizeDistance OptimizeStreamingMemory: %u -> %u bytes"), previous, available);
}