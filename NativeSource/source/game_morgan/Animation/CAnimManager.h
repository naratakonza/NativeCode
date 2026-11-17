#pragma once

struct RwStream;

class CAnimManager
{
public:
    static uintptr_t GetAnimationBlock(const char* name);
    static void LoadAnimFile(RwStream* stream, bool loadCompressed, char const(*uncompressedAnimations)[32]);
};