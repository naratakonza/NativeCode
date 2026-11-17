#include "../../Main.h"
#include "CPedIntelligence.h"

void CPedIntelligence::FlushImmediately(CPedIntelligence *thiz, const bool a2)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x269FF0), "\xFC\x6F\xBA\xA9\xFA\x67\x01\xA9\xF8\x5F\x02\xA9\xF6\x57\x03\xA9", 16, "__text");
    if (!thiz || !verified) {
        return;
    }

    reinterpret_cast<void (*)(CPedIntelligence*, bool)>(SA_ADDR(0x269FF0) + _dyld_get_image_vmaddr_slide(0))(thiz, a2);
}

void CPedIntelligence::SetTaskDuckSecondary(CPedIntelligence *thiz, uint16_t nTime)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x269908), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9\xFD\x7B\x02\xA9\xFD\x83\x00\x91", 16, "__text");
    if (!thiz || !verified) {
        return;
    }

    reinterpret_cast<void (*)(CPedIntelligence*, uint16_t)>(SA_ADDR(0x269908) + _dyld_get_image_vmaddr_slide(0))(thiz, nTime);
}

void CPedIntelligence::ClearTaskDuckSecondary(CPedIntelligence *thiz)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x269AC0), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9\xFD\x43\x00\x91\xF3\x03\x00\xAA", 16, "__text");
    if (!thiz || !verified) {
        return;
    }

    reinterpret_cast<void (*)(CPedIntelligence*)>(SA_ADDR(0x269AC0) + _dyld_get_image_vmaddr_slide(0))(thiz);
}