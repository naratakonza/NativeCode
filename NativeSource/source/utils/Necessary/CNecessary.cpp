#include "../../../Main.h"
#include "../Storage/CStorage.h"

#include <chrono>

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

std::size_t CNecessary::MPSTRLEN(const char* str)
{
    if (!str) {
        return 0;
    }

    std::size_t len = 0;
    while (*str != '\0')
    {
        len++;
        str++;
    }

    return len;
}

bool CNecessary::IsValidUTF8(const char* str, uint32_t maxLen)
{
    if (!str) {
        return false;
    }

    const unsigned char* bytes = (const unsigned char*)str;
    uint32_t i = 0;
    bool hasNonAscii = false;

    while ((maxLen == 0 || i < maxLen) && bytes[i] != '\0')
    {
        if (bytes[i] <= 0x7F)
        {
            i++;
            continue;
        }

        hasNonAscii = true;

        if (bytes[i] >= 0xC2 && bytes[i] <= 0xDF)
        {
            if ((maxLen > 0 && i + 1 >= maxLen) || bytes[i + 1] == '\0') return false;
            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        }
        else if (bytes[i] >= 0xE0 && bytes[i] <= 0xEF)
        {
            if ((maxLen > 0 && i + 2 >= maxLen) || bytes[i + 1] == '\0' || bytes[i + 2] == '\0') return false;

            if (bytes[i] == 0xE0 && bytes[i + 1] < 0xA0) return false;
            if (bytes[i] == 0xED && bytes[i + 1] > 0x9F) return false;

            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        }
        else if (bytes[i] >= 0xF0 && bytes[i] <= 0xF4)
        {
            if ((maxLen > 0 && i + 3 >= maxLen) || bytes[i + 1] == '\0' || bytes[i + 2] == '\0' || bytes[i + 3] == '\0') return false;

            if (bytes[i] == 0xF0 && bytes[i + 1] < 0x90) return false;
            if (bytes[i] == 0xF4 && bytes[i + 1] > 0x8F) return false;

            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            if ((bytes[i + 2] & 0xC0) != 0x80) return false;
            if ((bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        }
        else
        {
            return false;
        }
    }

    return hasNonAscii;
}

void CNecessary::ConvertLocalizedEncodingToUTF8(char* utf8Output, const char* cp874Input, uint32_t maxLen)
{
    if (!utf8Output || maxLen == 0) {
        return;
    }

    if (!cp874Input || *cp874Input == '\0' || maxLen == 1) {
        *utf8Output = '\0';
        return;
    }

    const uint32_t maxBytesAllowed = maxLen - 1;

    if (CNecessary::IsValidUTF8(cp874Input, 0))
    {
        uint32_t bytesWritten = 0;
        while (bytesWritten < maxBytesAllowed && cp874Input[bytesWritten]) {
            bytesWritten++;
        }
        
        while (bytesWritten > 0 && (static_cast<uint8_t>(cp874Input[bytesWritten]) & 0xC0) == 0x80) {
            bytesWritten--;
        }

        std::memcpy(utf8Output, cp874Input, bytesWritten);
        utf8Output += bytesWritten;

        *utf8Output = '\0';
        return;
    }

    static const uint32_t cp874Table[128] =
    {
        0xAC82E2, 0x0000, 0x0000, 0x0000, 0x0000, 0xA680E2, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x9880E2, 0x9980E2, 0x9C80E2, 0x9D80E2, 0xA280E2, 0x9380E2, 0x9480E2,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0xA0C2, 0x81B8E0, 0x82B8E0, 0x83B8E0, 0x84B8E0, 0x85B8E0, 0x86B8E0, 0x87B8E0,
        0x88B8E0, 0x89B8E0, 0x8AB8E0, 0x8BB8E0, 0x8CB8E0, 0x8DB8E0, 0x8EB8E0, 0x8FB8E0,
        0x90B8E0, 0x91B8E0, 0x92B8E0, 0x93B8E0, 0x94B8E0, 0x95B8E0, 0x96B8E0, 0x97B8E0,
        0x98B8E0, 0x99B8E0, 0x9AB8E0, 0x9BB8E0, 0x9CB8E0, 0x9DB8E0, 0x9EB8E0, 0x9FB8E0,
        0xA0B8E0, 0xA1B8E0, 0xA2B8E0, 0xA3B8E0, 0xA4B8E0, 0xA5B8E0, 0xA6B8E0, 0xA7B8E0,
        0xA8B8E0, 0xA9B8E0, 0xAAB8E0, 0xABB8E0, 0xACB8E0, 0xADB8E0, 0xAEB8E0, 0xAFB8E0,
        0xB0B8E0, 0xB1B8E0, 0xB2B8E0, 0xB3B8E0, 0xB4B8E0, 0xB5B8E0, 0xB6B8E0, 0xB7B8E0,
        0xB8B8E0, 0xB9B8E0, 0xBAB8E0, 0x0000, 0x0000, 0x0000, 0x0000, 0xBFB8E0,
        0x80B9E0, 0x81B9E0, 0x82B9E0, 0x83B9E0, 0x84B9E0, 0x85B9E0, 0x86B9E0, 0x87B9E0,
        0x88B9E0, 0x89B9E0, 0x8AB9E0, 0x8BB9E0, 0x8CB9E0, 0x8DB9E0, 0x8EB9E0, 0x8FB9E0,
        0x90B9E0, 0x91B9E0, 0x92B9E0, 0x93B9E0, 0x94B9E0, 0x95B9E0, 0x96B9E0, 0x97B9E0,
        0x98B9E0, 0x99B9E0, 0x9AB9E0, 0x9BB9E0, 0x0000, 0x0000, 0x0000, 0x0000
    };

    uint32_t bytesWritten = 0;

    while (*cp874Input)
    {
        if (bytesWritten >= maxBytesAllowed)
        {
            break;
        }

        if (*cp874Input & 0x80)
        {
            uint32_t utf8Value = cp874Table[static_cast<uint8_t>(*cp874Input++) & 0x7F];

            if (!utf8Value) {
                continue;
            }

            char b1 = static_cast<char>(utf8Value);
            char b2 = static_cast<char>(utf8Value >> 8);
            uint32_t tempVal = utf8Value >> 16;
            char b3 = (tempVal != 0) ? static_cast<char>(tempVal) : 0;

            uint32_t seqLen = (b3 != 0) ? 3 : (b2 != 0 ? 2 : 1);
            if (bytesWritten + seqLen > maxBytesAllowed) {
                break;
            }

            *utf8Output++ = b1;
            bytesWritten++;
            if (b2 != 0)
            {
                *utf8Output++ = b2;
                bytesWritten++;
            }

            if (b3 != 0)
            {
                *utf8Output++ = b3;
                bytesWritten++;
            }
        }
        else
        {
            *utf8Output++ = *cp874Input++;
            bytesWritten++;
        }
    }

    *utf8Output = '\0';
}
