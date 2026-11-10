#pragma once

#include "CStorageZip.h"
#include "PosixFileIO.h"

namespace morgan::fileio::storage {

struct InstallTrace {
    bool extracted = false;
    const char* validation = "not_run";
    const char* publish = "not_run";
};

struct RestoredAssets {
    FileDescriptor root{-1};
    const char* result = "not_found";
    bool accessible = false;
};

bool ValidateAssetRoot(int root, const SampChecks& checks);
FileDescriptor ValidateGameAssets(int morganRoot, const SampChecks& checks);
RestoredAssets RestoreGameAssets(int morganRoot, const SampChecks& checks);
bool HasRequiredBundleFiles(int bundle, std::size_t group);
GroupInventory InspectInstalledGroups(int morganRoot, const SampChecks& checks);
FileDescriptor ImportGameAssets(int morganRoot, std::FILE* zip, const SampChecks& checks,
                                Progress progress = nullptr, void* context = nullptr,
                                InstallTrace* trace = nullptr, CancelCheck cancelled = nullptr,
                                std::uint64_t* totalBytes = nullptr);

} // namespace morgan::fileio::storage
