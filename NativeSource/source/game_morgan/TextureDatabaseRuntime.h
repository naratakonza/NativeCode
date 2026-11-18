#pragma once

class TextureDatabaseRuntime
{
public:
    static void Register(uintptr_t toRegister);
    static void Unregister(uintptr_t toUnregister);
    static uintptr_t GetDatabase(const char* name);
    static uintptr_t GetTexture(const char* name);
    static uintptr_t GetTextureByName(const char* texture);
};