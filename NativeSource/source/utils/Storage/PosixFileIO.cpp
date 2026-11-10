#include "PosixFileIO.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace morgan::fileio {
namespace {

constexpr int DirectoryFlags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;

int ExactEntryError(int parent, const char* component, int opened) noexcept {
    struct stat openedStatus {};
    if (::fstat(opened, &openedStatus) != 0) return errno;
    // Open an independent directory description so readdir never shares a cursor with another call.
    FileDescriptor scan(::openat(parent, ".", DirectoryFlags));
    if (scan.get() < 0) return errno;
    DIR* directory = ::fdopendir(scan.get());
    if (!directory) return errno;
    scan.release();

    int error = 0;
    for (;;) {
        errno = 0;
        const dirent* entry = ::readdir(directory);
        if (!entry) {
            error = errno ? errno : EACCES;
            break;
        }
        if (std::strcmp(entry->d_name, component) == 0) {
            struct stat namedStatus {};
            if (::fstatat(parent, entry->d_name, &namedStatus, AT_SYMLINK_NOFOLLOW) != 0) {
                error = errno;
            } else if (namedStatus.st_dev != openedStatus.st_dev || namedStatus.st_ino != openedStatus.st_ino) {
                error = EACCES;
            }
            break;
        }
    }
    ::closedir(directory);
    return error;
}

int OpenExactAt(int parent, const char* component, int flags) noexcept {
    FileDescriptor descriptor(::openat(parent, component, flags));
    if (descriptor.get() < 0) return -1;
    const int error = ExactEntryError(parent, component, descriptor.get());
    if (error == 0) return descriptor.release();
    // Close before setting errno for the caller.
    ::close(descriptor.release());
    errno = error;
    return -1;
}

OpenResult SystemError(int error) noexcept {
    return {{}, std::error_code(error, std::generic_category())};
}

// Resolve a component without following links or picking a winner in a case collision.
int FindAssetName(int parent, std::string_view wanted, char (&name)[101]) noexcept {
    FileDescriptor scan(::openat(parent, ".", DirectoryFlags));
    if (scan.get() < 0) return errno;
    DIR* dir = ::fdopendir(scan.get());
    if (!dir) return errno;
    scan.release();

    bool found = false;
    int error = 0;
    for (;;) {
        errno = 0;
        const auto* entry = ::readdir(dir);
        if (!entry) {
            error = errno ? errno : found ? 0 : ENOENT;
            break;
        }
        const std::string_view candidate(entry->d_name);
        const auto equal = [](char a, char b) {
            const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
            return lower(a) == lower(b);
        };
        if (!std::equal(candidate.begin(), candidate.end(), wanted.begin(), wanted.end(), equal)) continue;
        if (found) { error = EACCES; break; }
        std::memcpy(name, candidate.data(), candidate.size() + 1);
        found = true;
    }
    ::closedir(dir);
    if (error) return error;
    struct stat status {};
    if (::fstatat(parent, name, &status, AT_SYMLINK_NOFOLLOW) != 0) return errno;
    return S_ISLNK(status.st_mode) ? ELOOP : 0;
}

} // namespace

int OpenExactDirectoryAt(int parent, const char* component) noexcept {
    if (!component || !*component || std::strpbrk(component, "/\\:") ||
        std::strcmp(component, ".") == 0 || std::strcmp(component, "..") == 0) {
        errno = EINVAL;
        return -1;
    }
    return OpenExactAt(parent, component, DirectoryFlags);
}

OpenResult OpenRegularFileAt(int parent, const char* leaf) noexcept {
    FileDescriptor file(OpenExactAt(parent, leaf, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    if (file.get() < 0) return SystemError(errno);
    struct stat status {};
    if (::fstat(file.get(), &status) != 0) return SystemError(errno);
    if (!S_ISREG(status.st_mode) || status.st_nlink != 1) return SystemError(EACCES);

    std::FILE* stream = ::fdopen(file.get(), "rb");
    if (!stream) return SystemError(errno);
    file.release();
    return {File(stream), {}};
}

OpenResult OpenSampLeafAt(int sampDirectory, Resource resource) noexcept {
    const auto* leaf = SampLeaf(resource);
    if (!leaf || sampDirectory < 0) return SystemError(EINVAL);
    return OpenRegularFileAt(sampDirectory, leaf);
}

OpenResult OpenSampAt(int trustedRoot, Resource resource) noexcept {
    if (trustedRoot < 0) return SystemError(EINVAL);
    const FileDescriptor directory(OpenExactDirectoryAt(trustedRoot, SampDirectory));
    if (directory.get() < 0) return SystemError(errno);
    return OpenSampLeafAt(directory.get(), resource);
}

OpenResult OpenExternalAssetAt(int trustedRoot, const Resolution& asset) noexcept {
    if (trustedRoot < 0) return SystemError(ENOENT);
    if (asset.length > asset.normalized.size()) return SystemError(EINVAL);
    auto path = asset.Path();
    if (!storage::AllowedAssetPath(path, asset.group)) return SystemError(EINVAL);
    FileDescriptor child(-1);
    int parent = trustedRoot;
    for (;;) {
        const auto slash = path.find('/');
        char name[101]{};
        if (const int error = FindAssetName(parent, path.substr(0, slash), name)) return SystemError(error);
        if (slash == std::string_view::npos) {
            auto result = OpenRegularFileAt(parent, name);
            if (!result.file) return result;
            struct stat status {};
            if (::fstat(::fileno(result.file.get()), &status) != 0) return SystemError(errno);
            if ((status.st_mode & 0777) != 0400 || status.st_size <= 0 ||
                static_cast<unsigned long long>(status.st_size) > (6ull << 30)) return SystemError(EACCES);
            return result;
        }
        child.reset(OpenExactDirectoryAt(parent, name));
        if (child.get() < 0) return SystemError(errno);
        parent = child.get();
        path.remove_prefix(slash + 1);
    }
}

} // namespace morgan::fileio
