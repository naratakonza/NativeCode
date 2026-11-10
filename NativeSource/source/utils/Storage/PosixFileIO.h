#pragma once

#include "../FileIO/FileIO.h"

#include <unistd.h>

namespace morgan::fileio {

class FileDescriptor {
public:
    explicit FileDescriptor(int fd) noexcept : fd_(fd) {}
    ~FileDescriptor() { reset(-1); }
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.release()) {}

    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) reset(other.release());
        return *this;
    }
    void reset(int fd) noexcept {
        if (fd_ >= 0) ::close(fd_);
        fd_ = fd;
    }

    int get() const noexcept { return fd_; }
    int release() noexcept {
        const int fd = fd_;
        fd_ = -1;
        return fd;
    }

private:
    int fd_;
};

inline constexpr char SampDirectory[] = "SAMP";

OpenResult OpenSampLeafAt(int sampDirectory, Resource resource) noexcept;
OpenResult OpenRegularFileAt(int parent, const char* leaf) noexcept;

// The caller owns the trusted directory descriptor. The returned stream owns its own descriptor.
OpenResult OpenSampAt(int trustedRoot, Resource resource) noexcept;
OpenResult OpenExternalAssetAt(int trustedRoot, const Resolution& asset) noexcept;

// For fixed, system-defined components supplied by the platform provider.
// Returns an owned directory descriptor or -1 with errno set.
int OpenExactDirectoryAt(int parent, const char* component) noexcept;


} // namespace morgan::fileio
