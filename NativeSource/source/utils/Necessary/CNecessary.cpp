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