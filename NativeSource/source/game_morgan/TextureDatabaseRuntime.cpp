#include "../../Main.h"
#include "TextureDatabaseRuntime.h"

#include <mach-o/dyld.h>

uintptr_t TextureDatabaseRuntime::GetDatabase(const char* name)
{
    if (!name) {
        return 0;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2C0760), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9", 8, "__text");
    if (!verified) {
        return 0;
    }

    return reinterpret_cast<uintptr_t (*)(const char*)>(SA_ADDR(0x2C0760) + _dyld_get_image_vmaddr_slide(0))(name);
}

void TextureDatabaseRuntime::Register(uintptr_t toRegister)
{
    if (!toRegister) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2BF208), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x2BF208) + _dyld_get_image_vmaddr_slide(0))(toRegister);
    }
}

void TextureDatabaseRuntime::Unregister(uintptr_t toUnregister)
{
    if (!toUnregister) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2BF2F4), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x2BF2F4) + _dyld_get_image_vmaddr_slide(0))(toUnregister);
    }
}

uintptr_t TextureDatabaseRuntime::GetTexture(const char* name)
{
    if (!name) {
        return 0;
    }

    // The first instruction may be a branch to the packaged GetTexture hook.
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2BF378), "\xFA\x67\x0A\xA9\xF8\x5F\x0B\xA9\xF6\x57\x0C\xA9\xF4\x4F\x0D\xA9", 16, "__text");
    if (!verified) {
        return 0;
    }

    return reinterpret_cast<uintptr_t (*)(const char*)>(SA_ADDR(0x2BF374) + _dyld_get_image_vmaddr_slide(0))(name);
}

uintptr_t TextureDatabaseRuntime::GetTextureByName(const char* texture)
{
    auto* rwTexture = reinterpret_cast<RwTexture*>(TextureDatabaseRuntime::GetTexture(texture));
    if (!rwTexture) {
        return 0;
    }

    RwTextureAddRefMacro(rwTexture);
    return reinterpret_cast<uintptr_t>(rwTexture);
}