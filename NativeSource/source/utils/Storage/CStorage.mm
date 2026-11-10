#import <Foundation/Foundation.h>

#include "CStorage.h"
#include "CStorageDisk.h"

#include <cstring>
#include <cstdlib>
#include <atomic>
#include <cerrno>
#include <copyfile.h>
#include <fcntl.h>
#include <mutex>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace morgan::fileio {

struct AssetRoot {
    explicit AssetRoot(AssetSource selected) : source(selected) {}
    FileDescriptor descriptor{-1};
    FileDescriptor externalDirectory{-1};
    AssetSource source;
};

namespace {

struct StorageState {
    std::mutex importMutex;
    std::shared_ptr<const AssetRoot> external;
    std::shared_ptr<const AssetRoot> active;
    std::shared_ptr<const storage::GroupInventory> groups;
    std::atomic<ImportStatus> importStatus{ImportStatus::NotImported};
    ExternalOpenDiagnostics externalOpens;
    std::atomic<bool> zipExtracted{false};
    std::atomic<const char*> validationResult{"not_run"};
    std::atomic<const char*> publishResult{"not_run"};
    std::atomic<const char*> restoreResult{"not_run"};
    std::atomic<bool> externalRootAccessible{false};
    std::atomic<std::uint64_t> bundleOpens{0};
};

StorageState& State() noexcept {
    static StorageState state;
    return state;
}

OpenResult SystemError(int error) noexcept {
    return {{}, std::error_code(error, std::generic_category())};
}

int OpenRoot(NSString* path) noexcept {
    const char* name = [path fileSystemRepresentation];
    if (!name || name[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    return ::open(name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
}

FileDescriptor OpenStorage(bool create) {
    NSFileManager* files = NSFileManager.defaultManager;
    NSURL* support = [files URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
    if (!support || (create && ![files createDirectoryAtURL:support withIntermediateDirectories:YES
                                                attributes:nil error:nullptr])) return FileDescriptor(-1);
    const FileDescriptor parent(OpenRoot(support.path));
    if (parent.get() < 0) return FileDescriptor(-1);
    if (create && ::mkdirat(parent.get(), "MorganGame", 0700) != 0 && errno != EEXIST) return FileDescriptor(-1);
    FileDescriptor root(OpenExactDirectoryAt(parent.get(), "MorganGame"));
    if (root.get() >= 0 && create) {
        NSURL* container = [support URLByAppendingPathComponent:@"MorganGame" isDirectory:YES];
        if (![container setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nullptr]) {
            return FileDescriptor(-1);
        }
    }
    return root;
}

const std::array<bool, storage::GroupCount>& BundleGroups() noexcept {
    static const auto groups = [] {
        @autoreleasepool {
            std::array<bool, storage::GroupCount> result{};
            const FileDescriptor bundle(OpenRoot(NSBundle.mainBundle.bundlePath));
            if (bundle.get() < 0) return result;
            for (std::size_t i = 0; i < result.size(); ++i) {
                try {
                    result[i] = storage::HasRequiredBundleFiles(bundle.get(), i);
                } catch (...) {
                    result[i] = false;
                }
            }
            return result;
        }
    }();
    return groups;
}

void RefreshGroups(StorageState& state, int parent) {
    auto groups = std::make_shared<const storage::GroupInventory>(
        storage::InspectInstalledGroups(parent, storage::SupportedAssets));
    std::atomic_store(&state.groups, std::move(groups));
}

struct ImportProgress {
    void (^callback)(double);
    bool (^cancelled)(void);
    std::uint64_t bytes = 0;
    std::uint64_t total = 0;
    int lastPercent = -1;
};

void AdvanceProgress(void* context, std::size_t bytes) {
    auto& progress = *static_cast<ImportProgress*>(context);
    progress.bytes += bytes;
    if (!progress.callback || !progress.total) return;
    const auto percent = static_cast<int>(100 * progress.bytes / progress.total);
    if (percent != progress.lastPercent) {
        progress.lastPercent = percent;
        progress.callback(static_cast<double>(progress.bytes) / static_cast<double>(progress.total));
    }
}

bool ImportCancelled(void* context) {
    const auto& progress = *static_cast<const ImportProgress*>(context);
    return progress.cancelled && progress.cancelled();
}

} // namespace

OpenResult CStorage::OpenBundleResource(const AssetRoot& root, Resource resource) noexcept
{
    if (root.source != AssetSource::Bundle) return SystemError(ENOENT);
    return OpenSampAt(root.descriptor.get(), resource);
}

OpenResult CStorage::OpenExternalResource(const AssetRoot& root, Resource resource) noexcept
{
    if (root.source != AssetSource::External) return SystemError(ENOENT);
    return OpenSampLeafAt(root.descriptor.get(), resource);
}


std::shared_ptr<const AssetRoot> CStorage::GetActiveAssetRoot() noexcept
{
    return std::atomic_load(&State().active);
}

std::string_view CStorage::GameBundleResourcePath() noexcept
{
    static const auto path = [] {
        std::array<char, MaxPathBytes + 1> result{};
        @autoreleasepool {
            const char* bytes = NSBundle.mainBundle.resourcePath.UTF8String;
            if (bytes && bytes[0] == '/') {
                const auto length = std::strlen(bytes);
                if (length <= MaxPathBytes) std::memcpy(result.data(), bytes, length);
            }
        }
        return result;
    }();
    return path.data();
}

OpenResult CStorage::OpenActiveResource(void*, const Resolution& asset) noexcept
{
    auto root = GetActiveAssetRoot();
    if (!root) return SystemError(ENOENT);
    // The selected generation is held for the session, so the pair never mixes sources.
    const auto resource = asset.resource;
    if (!SampLeaf(resource)) {
        if (asset.group == 0 && root->source != AssetSource::External) return SystemError(ENOENT);
        auto replacement = asset;
        if (root->source == AssetSource::External) ApplySampAssetOverride(replacement);
        auto result = OpenExternalAssetAt(root->externalDirectory.get(), replacement);
        if (asset.group == storage::DataGroup &&
            (replacement.Path() == "SAMP/tracks2.dat" || replacement.Path() == "SAMP/tracks4.dat") &&
            ((result.file && !HasSampTrackHeader(result.file.get())) ||
             result.error == std::errc::no_such_file_or_directory)) {
            // The iOS reader scans to a newline. Keep the original external track if the override is incomplete.
            result = OpenExternalAssetAt(root->externalDirectory.get(), asset);
        }
        if (result.file) result.origin = StreamOrigin::External;
        return result;
    }
    if (root->source == AssetSource::External) {
        auto result = OpenExternalResource(*root, resource);
        State().externalOpens.Record(resource, result);
        if (result.file) result.origin = StreamOrigin::External;
        return result;
    }
    auto result = OpenBundleResource(*root, resource);
    if (result.file) {
        State().bundleOpens.fetch_add(1, std::memory_order_relaxed);
        result.origin = StreamOrigin::Bundle;
    }
    return result;
}

std::error_code CStorage::ReadSampSettings(std::string& contents)
{
    @autoreleasepool {
        const std::lock_guard<std::mutex> lock(State().importMutex);
        const auto root = OpenStorage(false);
        if (root.get() < 0) return {errno, std::generic_category()};
        const FileDescriptor samp(OpenExactDirectoryAt(root.get(), SampDirectory));
        if (samp.get() < 0) return {errno, std::generic_category()};

        const auto input = OpenRegularFileAt(samp.get(), storage::SampSettingsName);
        if (!input.file) return input.error;
        struct stat info {};
        if (::fstat(::fileno(input.file.get()), &info) != 0) return {errno, std::generic_category()};
        if (info.st_uid != ::geteuid()) return std::make_error_code(std::errc::permission_denied);
        if (info.st_size < 0 || static_cast<std::uint64_t>(info.st_size) > storage::MaxSampSettingsBytes)
            return std::make_error_code(std::errc::file_too_large);

        contents.resize(static_cast<std::size_t>(info.st_size));
        if (std::fread(contents.data(), 1, contents.size(), input.file.get()) != contents.size() ||
            std::fgetc(input.file.get()) != EOF || std::ferror(input.file.get()))
            return std::make_error_code(std::errc::io_error);
        return {};
    }
}

bool CStorage::WriteSampSettings(std::string_view contents) noexcept
{
    if (contents.empty() || contents.size() > storage::MaxSampSettingsBytes) return false;
    @autoreleasepool {
        const std::lock_guard<std::mutex> lock(State().importMutex);
        const auto root = OpenStorage(true);
        if (root.get() < 0) return false;
        if (::mkdirat(root.get(), SampDirectory, 0700) != 0 && errno != EEXIST) return false;
        const FileDescriptor samp(OpenExactDirectoryAt(root.get(), SampDirectory));
        if (samp.get() < 0) return false;

        struct stat info {};
        if (::fstatat(samp.get(), storage::SampSettingsName, &info, AT_SYMLINK_NOFOLLOW) == 0)
        {
            if (!S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != ::geteuid()) return false;
        }
        else if (errno != ENOENT)
        {
            return false;
        }

        char temporary[32];
        std::snprintf(temporary, sizeof(temporary), ".settings-%08x%08x", ::arc4random(), ::arc4random());
        const FileDescriptor output(::openat(root.get(), temporary, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
        if (output.get() < 0) return false;

        auto remaining = contents;
        while (!remaining.empty())
        {
            const auto count = ::write(output.get(), remaining.data(), remaining.size());
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            remaining.remove_prefix(static_cast<std::size_t>(count));
        }

        const bool saved = remaining.empty() && ::fsync(output.get()) == 0 &&
                           ::renameat(root.get(), temporary, samp.get(), storage::SampSettingsName) == 0;
        if (!saved) ::unlinkat(root.get(), temporary, 0);
        return saved;
    }
}

bool CStorage::AppendSampLog(const char* message, bool truncate) noexcept
{
    @autoreleasepool {
        const std::unique_lock<std::mutex> lock(State().importMutex, std::try_to_lock);
        if (!lock.owns_lock()) return false;

        const auto root = OpenStorage(true);
        if (root.get() < 0) return false;
        if (::mkdirat(root.get(), SampDirectory, 0700) != 0 && errno != EEXIST) return false;

        const FileDescriptor samp(OpenExactDirectoryAt(root.get(), SampDirectory));
        if (samp.get() < 0) return false;
        FileDescriptor fd(::openat(samp.get(), storage::SampLogName, O_WRONLY | O_APPEND | O_CREAT | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC, 0600));
        struct stat info {};
        if (fd.get() < 0 || ::fstat(fd.get(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != ::geteuid()) return false;
        if (truncate && ::ftruncate(fd.get(), 0) != 0) return false;

        File log(::fdopen(fd.get(), "a"));
        if (!log) return false;
        fd.release();
        return std::fprintf(log.get(), "%s\n", message) >= 0 && std::fflush(log.get()) == 0;
    }
}

bool CStorage::CopySampLog(NSURL* destination) noexcept
{
    const std::lock_guard<std::mutex> lock(State().importMutex);
    const auto root = OpenStorage(false);
    if (root.get() < 0) return false;

    const FileDescriptor samp(OpenExactDirectoryAt(root.get(), SampDirectory));
    if (samp.get() < 0) return false;

    const auto input = OpenRegularFileAt(samp.get(), storage::SampLogName);
    struct stat info {};
    if (!input.file || ::fstat(::fileno(input.file.get()), &info) != 0 || info.st_uid != ::geteuid()) return false;

    const char* path = destination.fileSystemRepresentation;
    if (!path) return false;

    FileDescriptor output(::open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (output.get() < 0) return false;

    if (::fcopyfile(::fileno(input.file.get()), output.get(), nullptr, COPYFILE_DATA) == 0) return true;
    output.reset(-1);
    ::unlink(path);
    return false;
}

void CStorage::ValidateExternalAssets() noexcept
{
    @autoreleasepool {
        auto& state = State();
        const std::lock_guard<std::mutex> lock(state.importMutex);
        try {
            const auto parent = OpenStorage(false);
            if (parent.get() < 0) {
                const bool missing = errno == ENOENT;
                std::atomic_store(&state.external, std::shared_ptr<const AssetRoot>{});
                std::atomic_store(&state.groups, std::shared_ptr<const storage::GroupInventory>{});
                state.externalRootAccessible.store(false);
                state.validationResult.store("not_run");
                state.restoreResult.store(missing ? "not_found" : "storage_unavailable");
                state.importStatus.store(missing ? ImportStatus::NotImported : ImportStatus::Failed);
                return;
            }
            auto root = std::make_shared<AssetRoot>(AssetSource::External);
            auto restored = storage::RestoreGameAssets(parent.get(), storage::SupportedAssets);
            RefreshGroups(state, parent.get());
            state.restoreResult.store(restored.result);
            state.externalRootAccessible.store(true);
            if (restored.root.get() >= 0) {
                root->descriptor = std::move(restored.root);
                root->externalDirectory.reset(::fcntl(parent.get(), F_DUPFD_CLOEXEC, 0));
                std::atomic_store(&state.external, std::shared_ptr<const AssetRoot>(std::move(root)));
                state.validationResult.store("passed");
                if (std::string_view(restored.result) == "migrated") state.publishResult.store("published");
                state.importStatus.store(ImportStatus::Ready);
                return;
            }
            std::atomic_store(&state.external, std::shared_ptr<const AssetRoot>{});
            const bool missing = std::string_view(restored.result) == "not_found";
            state.validationResult.store(missing ? "not_run" : "failed");
            bool hasOtherAssets = false;
            if (const auto groups = std::atomic_load(&state.groups)) {
                for (std::size_t i = 1; i < groups->size(); ++i) hasOtherAssets |= (*groups)[i].valid;
            }
            state.importStatus.store(missing ? (hasOtherAssets ? ImportStatus::Ready : ImportStatus::NotImported)
                                             : ImportStatus::Failed);
        } catch (...) {
            std::atomic_store(&state.external, std::shared_ptr<const AssetRoot>{});
            state.externalRootAccessible.store(false);
            state.restoreResult.store("restore_failed");
            state.validationResult.store("failed");
            state.importStatus.store(ImportStatus::Failed);
        }
    }
}

std::error_code CStorage::SelectSampAssets(bool externalOnly) noexcept
{
    try {
        auto& state = State();
        const std::lock_guard<std::mutex> lock(state.importMutex);
        if (externalOnly) {
            if (!IsAssetFreeBundle() || !ExternalRuntimeAssetsReady(GetAssetStatus())) {
                return std::make_error_code(std::errc::operation_not_permitted);
            }
            auto pinned = std::make_shared<AssetRoot>(AssetSource::External);
            pinned->externalDirectory = OpenStorage(false);
            if (pinned->externalDirectory.get() < 0) return {errno ? errno : EIO, std::generic_category()};
            RefreshGroups(state, pinned->externalDirectory.get());
            pinned->descriptor = storage::ValidateGameAssets(pinned->externalDirectory.get(),
                                                             storage::SupportedAssets);
            if (pinned->descriptor.get() < 0 || !ExternalRuntimeAssetsReady(GetAssetStatus())) {
                return std::make_error_code(std::errc::illegal_byte_sequence);
            }
            std::atomic_store(&state.active, std::shared_ptr<const AssetRoot>(std::move(pinned)));
            return {};
        }
        auto root = std::atomic_load(&state.external);
        if (root && !storage::ValidateAssetRoot(root->descriptor.get(), storage::SupportedAssets)) {
            if (std::atomic_compare_exchange_strong(&state.external, &root, std::shared_ptr<const AssetRoot>{})) {
                state.validationResult.store("failed");
                state.importStatus.store(ImportStatus::Failed);
                root.reset();
            }
        }
        if (!root) {
            @autoreleasepool {
                auto bundle = std::make_shared<AssetRoot>(AssetSource::Bundle);
                bundle->descriptor.reset(OpenRoot(NSBundle.mainBundle.bundlePath));
                if (bundle->descriptor.get() < 0) return {errno, std::generic_category()};
                for (std::size_t i = 0; i < storage::AssetResources.size(); ++i) {
                    auto file = OpenBundleResource(*bundle, storage::AssetResources[i]);
                    if (!file.file) return file.error ? file.error : std::make_error_code(std::errc::io_error);
                    if (!storage::MatchesAsset(file.file.get(), storage::SupportedAssets[i])) {
                        return std::make_error_code(std::errc::illegal_byte_sequence);
                    }
                }
                bundle->externalDirectory = OpenStorage(false);
                root = std::move(bundle);
            }
        }
        std::atomic_store(&state.active, std::move(root));
        return {};
    } catch (const std::bad_alloc&) {
        return std::make_error_code(std::errc::not_enough_memory);
    } catch (...) {
        return std::make_error_code(std::errc::io_error);
    }
}

void CStorage::UseOriginalAssets() noexcept
{
    std::atomic_store(&State().active, std::shared_ptr<const AssetRoot>{});
}

AssetStatus CStorage::GetAssetStatus() noexcept
{
    auto& state = State();
    auto active = GetActiveAssetRoot();
    const bool ready = std::atomic_load(&state.external) != nullptr;
    const auto groups = std::atomic_load(&state.groups);
    std::uint64_t installedBytes = 0;
    if (groups) for (const auto& group : *groups) installedBytes += group.bytes;
    return {active ? active->source : AssetSource::Original, state.importStatus.load(), ready, installedBytes,
            state.externalOpens.Snapshot(), state.bundleOpens.load(std::memory_order_relaxed),
            state.zipExtracted.load(), state.validationResult.load(), state.publishResult.load(),
            state.restoreResult.load(), state.externalRootAccessible.load(),
            groups ? *groups : storage::GroupInventory{}, BundleGroups()};
}

bool CStorage::IsAssetFreeBundle() noexcept
{
    static const bool assetFree = [] {
        const FileDescriptor root(OpenRoot(NSBundle.mainBundle.bundlePath));
        if (root.get() < 0) return false;
        for (const auto* group : storage::GroupNames) {
            struct stat info {};
            if (::fstatat(root.get(), group, &info, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT) return false;
        }
        return true;
    }();
    return assetFree;
}

NSString* CStorage::LaunchBlocker(bool samp, bool externalOnly)
{
    if (externalOnly && !IsAssetFreeBundle()) return @"ExternalOnly ใช้กับชุดทดสอบ Full External เท่านั้น";
    const auto status = GetAssetStatus();
    NSMutableArray<NSString*>* missing = [NSMutableArray array];
    NSMutableArray<NSString*>* unsupported = [NSMutableArray array];
    for (std::size_t i = (samp || externalOnly) ? 0 : 1; i < storage::GroupCount; ++i) {
        const auto readiness = ClassifyGroupForLaunch(status, i, externalOnly);
        if (readiness == GroupLaunchState::Bundle || readiness == GroupLaunchState::External) continue;
        NSString* group = [NSString stringWithUTF8String:storage::GroupNames[i]];
        if (readiness == GroupLaunchState::Missing) [missing addObject:group];
        if (readiness == GroupLaunchState::Unsupported) [unsupported addObject:group];
    }
    if (missing.count) {
        return [NSString stringWithFormat:@"Missing Assets · %@\nนำเข้า ZIP จาก Files ก่อนเริ่มเกม",
                                         [missing componentsJoinedByString:@", "]];
    }
    if (unsupported.count) {
        return [NSString stringWithFormat:@"Unsupported Loader · %@\nรุ่นทดลองนี้ยังเริ่มเกมจากกลุ่มเหล่านี้ไม่ได้",
                                         [unsupported componentsJoinedByString:@", "]];
    }
    if (externalOnly && !ExternalRuntimeAssetsReady(status)) {
        return @"Game Assets ภายนอกยังไม่ผ่านการตรวจสำหรับ ExternalOnly";
    }
    return nil;
}

NSString* CStorage::ImportZip(NSURL* url, void (^progress)(double), void (^onReadable)(void),
                              bool (^cancelled)(void))
{
    auto* state = &State();
    const std::lock_guard<std::mutex> lock(state->importMutex);
    if (std::atomic_load(&state->active)) return @"ปิดเกมก่อนเปลี่ยน Game Assets";
    state->importStatus.store(ImportStatus::Validating);
    state->zipExtracted.store(false);
    state->validationResult.store("not_run");
    state->publishResult.store("not_run");
    if (!url.isFileURL) {
        state->importStatus.store(ImportStatus::Failed);
        return @"เลือกไฟล์ ZIP ผ่านแอป Files";
    }
    const BOOL scoped = [url startAccessingSecurityScopedResource];
    __block NSString* message = @"ไม่สามารถอ่าน ZIP ที่เลือกได้";
    __block storage::InstallTrace trace;
    @try {
        NSFileCoordinator* coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
        [coordinator coordinateReadingItemAtURL:url options:0 error:nullptr byAccessor:^(NSURL* coordinated) {
            try {
                const char* path = coordinated.fileSystemRepresentation;
                FileDescriptor fd(path ? ::open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC) : -1);
                struct stat info {};
                if (fd.get() < 0 || ::fstat(fd.get(), &info) != 0 || !S_ISREG(info.st_mode)) {
                    throw std::runtime_error("Cannot open ZIP as a regular file");
                }
                File zip(::fdopen(fd.get(), "rb"));
                if (!zip) throw std::runtime_error("Cannot read ZIP");
                fd.release();
                if (onReadable) onReadable();
                const auto parent = OpenStorage(true);
                if (parent.get() < 0) throw std::runtime_error("Cannot prepare Application Support");
                auto root = std::make_shared<AssetRoot>(AssetSource::External);
                ImportProgress updates{progress, cancelled};
                root->descriptor = storage::ImportGameAssets(parent.get(), zip.get(), storage::SupportedAssets,
                                                              AdvanceProgress, &updates, &trace, ImportCancelled, &updates.total);
                const bool ready = root->descriptor.get() >= 0;
                if (ready) root->externalDirectory.reset(::fcntl(parent.get(), F_DUPFD_CLOEXEC, 0));
                state->externalRootAccessible.store(true);
                std::atomic_store(&state->external, ready ? std::shared_ptr<const AssetRoot>(std::move(root))
                                                          : std::shared_ptr<const AssetRoot>{});
                RefreshGroups(*state, parent.get());
                message = nil;
            } catch (const std::runtime_error& error) {
                message = [NSString stringWithUTF8String:error.what()];
            } catch (...) {
                message = @"ไม่สามารถตรวจสอบหรือติดตั้ง Assets ได้";
            }
        }];
    } @finally {
        if (scoped) [url stopAccessingSecurityScopedResource];
        state->zipExtracted.store(trace.extracted);
        state->validationResult.store(trace.validation);
        state->publishResult.store(trace.publish);
        state->importStatus.store(message ? ImportStatus::Failed : ImportStatus::Ready);
    }
    return message;
}

} // namespace morgan::fileio
