#include "../../Main.h"
#include "CTxdStore.h"

#include <mach-o/dyld.h>

void CTxdStore::PushCurrentTxd()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x316394), "\xC8\x2E\x00\xB0\x08\x2D\x43\xF9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)()>(SA_ADDR(0x316394) + _dyld_get_image_vmaddr_slide(0))();
    }
}

void CTxdStore::PopCurrentTxd()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3163B8), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)()>(SA_ADDR(0x3163B8) + _dyld_get_image_vmaddr_slide(0))();
    }
}

int CTxdStore::FindTxdSlot(const char* name)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x316018), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91", 8, "__text");
    if (!verified) {
        return -1;
    }

    return reinterpret_cast<int (*)(const char*)>(SA_ADDR(0x316018) + _dyld_get_image_vmaddr_slide(0))(name);
}

void CTxdStore::SetCurrentTxd(int index)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3162D0), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(int, const char*)>(SA_ADDR(0x3162D0) + _dyld_get_image_vmaddr_slide(0))(index, nullptr);
    }
}

int CTxdStore::AddTxdSlot(const char* szTxdName, const char* szTexdb)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x315BC8), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9", 8, "__text");
    if (!verified) {
        return -1;
    }

    return reinterpret_cast<int (*)(const char*, const char*, bool)>(SA_ADDR(0x315BC8) + _dyld_get_image_vmaddr_slide(0))(szTxdName, szTexdb, false);
}

void CTxdStore::AddRef(int iTxdSlot)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x316108), "\xC8\x2E\x00\xB0\x08\x29\x43\xF9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(int)>(SA_ADDR(0x316108) + _dyld_get_image_vmaddr_slide(0))(iTxdSlot);
    }
}

RwTexDictionary* CTxdStore::GetTxdParent(RwTexDictionary* pTxd)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x31874C), "\x88\x0E\x00\x90\x08\x89\x8E\xB9\x00\x68\x68\xF8\xC0\x03\x5F\xD6", 16, "__text");
    if (!verified) {
        return nullptr;
    }

    return reinterpret_cast<RwTexDictionary* (*)(RwTexDictionary*)>(SA_ADDR(0x31874C) + _dyld_get_image_vmaddr_slide(0))(pTxd);
}