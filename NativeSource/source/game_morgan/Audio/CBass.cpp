#include "../../../Main.h"
#include "CBass.h"

#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <limits.h>

#define MORGAN_BASS_API(result, name, args) result (*name) args = nullptr;
#include "CBassApi.inc"
#undef MORGAN_BASS_API

bool CBass::Initialise()
{
    static const bool initialised = []
    {
        const auto bundle = CFBundleGetMainBundle();
        const auto frameworks = bundle ? CFBundleCopyPrivateFrameworksURL(bundle) : nullptr;
        if (!frameworks)
        {
            CNecessary::Log(OBFUSCATE("CBass: app Frameworks directory unavailable"));
            return false;
        }

        UInt8 directory[PATH_MAX];
        const bool hasPath = CFURLGetFileSystemRepresentation(frameworks, true, directory, sizeof(directory));
        CFRelease(frameworks);
        if (!hasPath)
        {
            CNecessary::Log(OBFUSCATE("CBass: cannot read app Frameworks path"));
            return false;
        }

        const auto path = std::string(reinterpret_cast<const char*>(directory)) + "/bass.framework/bass";
        void* const library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!library)
        {
            const char* error = dlerror();
            CNecessary::Log(OBFUSCATE("CBass: %s"), error ? error : "bass.framework unavailable");
            return false;
        }

        const auto getVersion = reinterpret_cast<std::uint32_t (*)()>(dlsym(library, "BASS_GetVersion"));
        const auto version = getVersion ? getVersion() : 0;
        if ((version >> 16) != 0x0204)
        {
            CNecessary::Log(OBFUSCATE("CBass: incompatible BASS API version 0x%08X"), version);
            dlclose(library);
            return false;
        }

#define MORGAN_BASS_API(result, name, args) \
        const auto resolved_##name = reinterpret_cast<decltype(name)>(dlsym(library, #name)); \
        if (!resolved_##name) \
        { \
            CNecessary::Log(OBFUSCATE("CBass: missing %s"), #name); \
            dlclose(library); \
            return false; \
        }
#include "CBassApi.inc"
#undef MORGAN_BASS_API

        // Keep the framework loaded for the lifetime of the published pointers.
#define MORGAN_BASS_API(result, name, args) name = resolved_##name;
#include "CBassApi.inc"
#undef MORGAN_BASS_API

        CNecessary::Log(OBFUSCATE("CBass Initialise: BASS 0x%08X"), version);
        return true;
    }();

    return initialised;
}