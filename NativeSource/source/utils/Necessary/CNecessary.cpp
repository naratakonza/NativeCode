#include "../../../Main.h"
#include "../Storage/CStorage.h"

#include <chrono>
#include <CoreFoundation/CoreFoundation.h>

void CNecessary::Log(const char* fmt, ...)
{
    if (!fmt) {
        return;
    }

    const int originalErrno = errno;
    char buffer[0xFF]{};
    va_list args;
    va_start(args, fmt);
    const int count = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (count >= 0)
    {
        static std::mutex logMutex;
        const std::lock_guard<std::mutex> lock(logMutex);
        morgan::fileio::CStorage::AppendSampLog(buffer);
        std::fprintf(stderr, "[Morgan] %s\n", buffer);
    }

    errno = originalErrno;
}

std::uint32_t CNecessary::GetTickCount()
{
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

std::uint32_t CNecessary::GetLocalizedEncoding()
{
    auto* settings = CSetting::GetConfig();
    const int language = settings ? settings->GetIntValue(OBFUSCATE("Language")) : 0;
    CFStringRef charset = CFSTR("windows-1252");
    switch (language)
    {
        case 1:
        case 7: charset = CFSTR("windows-874"); break;
        case 2: charset = CFSTR("windows-1251"); break;
        case 5: charset = CFSTR("windows-1258"); break;
        case 8:
        case 9: charset = CFSTR("windows-1256"); break;
        case 10: charset = CFSTR("windows-1250"); break;
    }
    return CFStringConvertIANACharSetNameToEncoding(charset);
}

void CNecessary::ConvertLocalizedEncodingToUTF8(char* utf8Output, const char* input, std::uint32_t maxLen)
{
    if (!utf8Output || !maxLen) {
        return;
    }
    utf8Output[0] = '\0';
    if (!input || !*input) {
        return;
    }

    CFStringRef text = CFStringCreateWithCString(kCFAllocatorDefault, input, kCFStringEncodingUTF8);
    if (!text) {
        text = CFStringCreateWithCString(kCFAllocatorDefault, input, CNecessary::GetLocalizedEncoding());
    }
    if (!text) {
        text = CFStringCreateWithCString(kCFAllocatorDefault, input, kCFStringEncodingISOLatin1);
    }
    if (!text) {
        return;
    }

    CFIndex length = 0;
    CFStringGetBytes(text, CFRangeMake(0, CFStringGetLength(text)), kCFStringEncodingUTF8, 0, false,
                     reinterpret_cast<UInt8*>(utf8Output), maxLen - 1, &length);
    utf8Output[length] = '\0';
    CFRelease(text);
}
