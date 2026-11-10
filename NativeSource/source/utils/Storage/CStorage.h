#pragma once

#include "CStorageZip.h"

#include <atomic>
#include <memory>
#include <string>

#if defined(__OBJC__)
@class NSURL, NSString;
#endif

namespace morgan::fileio {

enum class AssetSource { Original, Bundle, External };
enum class ImportStatus { NotImported, Validating, Ready, Failed };

struct ExternalOpenSnapshot {
    std::uint64_t mainOpens;
    std::uint64_t imageOpens;
    std::uint64_t failures;
    const char* lastError;

    std::uint64_t Successes() const noexcept { return mainOpens + imageOpens; }
    std::uint64_t Attempts() const noexcept { return Successes() + failures; }
};

class ExternalOpenDiagnostics {
public:
    void Record(Resource resource, const OpenResult& result) noexcept {
        if (result.file && resource == Resource::MainScript) {
            main_.fetch_add(1, std::memory_order_relaxed);
        } else if (result.file && resource == Resource::ScriptImage) {
            image_.fetch_add(1, std::memory_order_relaxed);
        } else {
            const char* category = "io";
            if (result.error == std::errc::no_such_file_or_directory) category = "missing";
            if (result.error == std::errc::permission_denied) category = "permission";
            if (result.error == std::errc::too_many_symbolic_link_levels) category = "unsafe_link";
            if (result.error == std::errc::invalid_argument) category = "invalid_request";
            error_.store(category, std::memory_order_relaxed);
            failures_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    ExternalOpenSnapshot Snapshot() const noexcept {
        return {main_.load(std::memory_order_relaxed), image_.load(std::memory_order_relaxed),
                failures_.load(std::memory_order_relaxed), error_.load(std::memory_order_relaxed)};
    }

private:
    std::atomic<std::uint64_t> main_{0};
    std::atomic<std::uint64_t> image_{0};
    std::atomic<std::uint64_t> failures_{0};
    std::atomic<const char*> error_{"none"};
};

struct AssetStatus {
    AssetSource source;
    ImportStatus importStatus;
    bool externalReady;
    std::uint64_t importedBytes;
    ExternalOpenSnapshot externalOpens;
    std::uint64_t bundleFallbackOpens;
    bool zipExtracted;
    const char* validationResult;
    const char* publishResult;
    const char* restoreResult;
    bool externalRootAccessible;
    storage::GroupInventory groups;
    std::array<bool, storage::GroupCount> bundleGroups;
};

enum class GroupLaunchState { Bundle, External, Missing, Unsupported };

inline GroupLaunchState ClassifyGroupForLaunch(const AssetStatus& status, std::size_t group,
                                                  bool externalOnly = false) noexcept {
    if (group >= storage::GroupCount) return GroupLaunchState::Missing;
    if (!externalOnly && status.bundleGroups[group]) return GroupLaunchState::Bundle;
    if (!status.groups[group].valid || (group == 0 && !status.externalReady)) return GroupLaunchState::Missing;
    return externalOnly || group == 0 ? GroupLaunchState::External : GroupLaunchState::Unsupported;
}

inline bool ExternalRuntimeAssetsReady(const AssetStatus& status) noexcept {
    const std::string_view validation = status.validationResult ? status.validationResult : "";
    const std::string_view publish = status.publishResult ? status.publishResult : "";
    const std::string_view restore = status.restoreResult ? status.restoreResult : "";
    const bool installed = (status.zipExtracted && publish == "published") ||
                           restore == "restored" || restore == "migrated";
    if (status.importStatus != ImportStatus::Ready || !status.externalRootAccessible ||
        validation != "passed" || !installed) return false;
    for (std::size_t i = 0; i < storage::GroupCount; ++i) {
        if (status.bundleGroups[i] || ClassifyGroupForLaunch(status, i, true) != GroupLaunchState::External) {
            return false;
        }
    }
    return true;
}

struct AssetRoot;

class CStorage
{
public:
    static OpenResult OpenBundleResource(const AssetRoot& root, Resource resource) noexcept;
    static OpenResult OpenExternalResource(const AssetRoot& root, Resource resource) noexcept;
    static std::string_view GameBundleResourcePath() noexcept;
    static OpenResult OpenActiveResource(void* context, const Resolution& asset) noexcept;

    static std::error_code ReadSampSettings(std::string& contents);
    static bool WriteSampSettings(std::string_view contents) noexcept;
    static bool AppendSampLog(const char* message) noexcept;
    static void ValidateExternalAssets() noexcept;
    static std::error_code SelectSampAssets(bool externalOnly = false) noexcept;
    static void UseOriginalAssets() noexcept;
    static AssetStatus GetAssetStatus() noexcept;
    static bool IsAssetFreeBundle() noexcept;

#if defined(__OBJC__)
    // Call on a background queue. The scope and coordinated read live until this method returns.
    static NSString* ImportZip(NSURL* url, void (^progress)(double), void (^onReadable)(void),
                               bool (^cancelled)(void));
    static void MarkSampLogGameStarted() noexcept;
    static bool CopySampLog(NSURL* destination, bool previousGame = false) noexcept;
    static NSString* LaunchBlocker(bool samp, bool externalOnly = false);
#endif

private:
    static std::shared_ptr<const AssetRoot> GetActiveAssetRoot() noexcept;
};

} // namespace morgan::fileio
