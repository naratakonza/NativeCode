#include "../../Main.h"
#include "CEntryExitManager.h"
#include "CEntryExit.h"

#include <mach-o/dyld.h>

void CEntryExitManager::DisableInteriorEnterExits()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0xB4CF8), "\xC8\x2F\x00\xD0\x08\x79\x47\xF9\x09\x11\x40\xB9", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0xB4D30), "\x0A\x01\x40\xF9\x4A\x19\x09\xAB\xE0\xFE\xFF\x54\x4B\x61\x40\x79", 16, "__text");
    if (!verified || !pthread_main_np()) {
        return;
    }

    // CPool<CEntryExit>: object อยู่ที่ +0, flag ของ slot อยู่ที่ +8, ความจุอยู่ที่ +0x10
    struct EntryExitPool
    {
        CEntryExit* entries;
        const uint8_t* slotFlags;
        uint32_t count;
    };

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    const auto* poolEntryExit = *reinterpret_cast<EntryExitPool* const*>(SA_ADDR(0x6AEEF0) + slide);
    if (!poolEntryExit) {
        return;
    }

    for (uint32_t i = 0; i < poolEntryExit->count; ++i)
    {
        poolEntryExit->entries[i].flags = 0;
    }
}