#pragma once

#include "HookAdapter.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string_view>

enum class StartupPhase : std::uint8_t {
    None, FrameworkRegistered, UIKitLaunchFinished, TickGateInstalled, TickGateRejected, FirstTickDeferred,
    GTAWindowObserved, LauncherPresented, StorageValidationStarted, StorageValidationFinished,
    ImportStarted, ImportFailed, ImportCompleted, HandoffCompleted,
    ExternalModeSelected, TickGateReleased, FirstTickStarted, FirstTickCompleted
};

enum class StartupReadResult { None, InFlight, Opened, Missing, Failed };
using StartupAssetPath = std::array<char, morgan::fileio::storage::MaxAssetPathBytes + 1>;

struct StartupRead {
    std::uint64_t sequence = 0;
    StartupAssetPath path{};
    std::size_t group = morgan::fileio::storage::GroupCount;
    bool external = false;
};

enum class RuntimeOpenRoute {
    OriginalOutsidePolicy, OriginalAccess, OriginalEndpointInactive,
    PathRejected, AccessRejected, ExternalOpened, ExternalFailed, BridgeFailed
};
using RuntimePathKind = morgan::fileio::AssetPathKind;
enum class RuntimeOpenMode { Read, Write, Update, Other };

struct RuntimeOpenEvent {
    RuntimeOpenRoute route;
    RuntimePathKind pathKind;
    RuntimeOpenMode mode;
    std::uint32_t access;
    int nativeError;
    bool opened;
    std::size_t group;
    morgan::fileio::Disposition decision;
    morgan::fileio::AssetPathIssue issue;
    bool usedResolvedPath;
    morgan::fileio::ResolvedPathKind resolvedKind;
    morgan::fileio::PathRejection rejection;
    bool stockTrailingSpace;
    StartupAssetPath path{};
    StartupAssetPath trustedResolvedSuffix{};
};

struct StartupSnapshot {
    StartupPhase lastPhase = StartupPhase::None;
    std::uint32_t phases = 0;
    std::uint64_t readAttempts = 0;
    std::uint64_t runtimeReadAttempts = 0;
    std::uint64_t droppedEvents = 0;
    StartupRead lastRead{};
    StartupReadResult readResult = StartupReadResult::None;
    int nativeError = 0;
    StartupRead lastFailure{};
    int lastFailureErrno = 0;
    std::size_t lastSuccessfulExternalGroup = morgan::fileio::storage::GroupCount;
    std::array<StartupAssetPath, 8> missingPaths{};
    std::size_t missingCount = 0;
    std::array<RuntimeOpenEvent, 16> runtimeOpenEvents{};
    std::uint64_t runtimeOpenEventCount = 0;

    bool Saw(StartupPhase phase) const noexcept {
        return (phases & (std::uint32_t{1} << static_cast<unsigned>(phase))) != 0;
    }

    bool CanReleaseExternalTick() const noexcept {
        return Saw(StartupPhase::TickGateInstalled) && Saw(StartupPhase::FirstTickDeferred) &&
               !Saw(StartupPhase::TickGateReleased);
    }
};

// Bounded telemetry. The optional failure sink is called after the ring lock is released.
class StartupTrace {
public:
    void Note(StartupPhase phase) noexcept;
    StartupRead BeginRead(const char* logicalPath, bool externalOnly = false,
                          const morgan::fileio::Resolution* asset = nullptr) noexcept;
    void FinishRead(const StartupRead& read, bool opened, int error) noexcept;
    using FailureSink = void (*)(const RuntimeOpenEvent&) noexcept;
    void SetFailureSink(FailureSink sink) noexcept { failureSink_.store(sink); }
    void RecordRuntimeOpen(std::string_view path, const char* mode, std::uint32_t access,
                           RuntimeOpenRoute route, bool opened, int error,
                           const morgan::fileio::GameAssetPath* classified = nullptr,
                           std::string_view resolvedPath = {}) noexcept;
    StartupSnapshot Snapshot() noexcept;

private:
    std::atomic<std::uint64_t> sequence_{0};
    std::atomic<std::uint64_t> readAttempts_{0};
    std::atomic<std::uint64_t> runtimeReadAttempts_{0};
    std::atomic<std::uint64_t> droppedEvents_{0};
    std::atomic<std::uint32_t> phases_{0};
    std::atomic<StartupPhase> lastPhase_{StartupPhase::None};
    std::mutex lock_;
    std::atomic<FailureSink> failureSink_{nullptr};
    StartupSnapshot reads_;
    std::uint64_t lastExternalSuccessSequence_ = 0;
};

StartupTrace& GameStartupTrace() noexcept;

enum class StreamSelection {
    Waiting, WaitingAfterAssetRead, EarlyRead, OriginalOnly, OriginalOnlyAfterEarlyRead, SampReadOnly, ExternalOnly
};
enum class StreamError { None, InvalidRequest, RecursiveOpen, MissingReplacement, ReplacementRejected,
                         OriginalOpenFailed, AdapterError };

struct GroupReadSnapshot {
    std::uint64_t attempts;
    std::uint64_t successes;
    std::uint64_t failures;
    std::uint64_t fallbacks;
    std::uint64_t earlyReads;
    std::uint64_t externalSuccesses;
    std::uint64_t bundleSuccesses;
};

struct StreamSnapshot
{
    std::uint64_t bridgeEntryCalls;
    std::uint64_t originalFopenCalls;
    std::uint64_t mainScriptRedirects;
    std::uint64_t scriptImageRedirects;
    std::uint64_t redirectFailures;
    StreamError lastError;
    StreamSelection selection;
    std::array<GroupReadSnapshot, morgan::fileio::storage::GroupCount> groups{};
};

// The iOS C entry owns the production instance. Selection linearizes against the
// first supported script read, even if the original GTA window is still covered.
class GameStreamDiagnostics
{
public:
    bool BeginOpen(const char* stdioMode, const char* logicalPath, std::uint32_t access,
                   const morgan::fileio::Resolution* asset = nullptr) noexcept;
    bool SelectSamp() noexcept;
    bool SelectExternalOnly() noexcept;
    StreamSelection Selection() const noexcept { return selection_.load(); }
    void SelectOriginal() noexcept;
    void RecordOriginalCall() noexcept;
    void RecordError(StreamError error) noexcept;
    void RecordRedirect(const morgan::fileio::HookResult& result) noexcept;
    StreamSnapshot Snapshot() const noexcept;

private:
    struct GroupReads {
        std::atomic<std::uint64_t> attempts{0};
        std::atomic<std::uint64_t> successes{0};
        std::atomic<std::uint64_t> failures{0};
        std::atomic<std::uint64_t> fallbacks{0};
        std::atomic<std::uint64_t> earlyReads{0};
        std::atomic<std::uint64_t> externalSuccesses{0};
        std::atomic<std::uint64_t> bundleSuccesses{0};
    };
    std::array<GroupReads, morgan::fileio::storage::GroupCount> groups_{};
    std::atomic<std::uint64_t> bridgeEntryCalls_{0};
    std::atomic<std::uint64_t> originalFopenCalls_{0};
    std::atomic<std::uint64_t> mainScriptRedirects_{0};
    std::atomic<std::uint64_t> scriptImageRedirects_{0};
    std::atomic<std::uint64_t> redirectFailures_{0};
    std::atomic<StreamError> lastError_{StreamError::None};
    std::atomic<StreamSelection> selection_{StreamSelection::Waiting};
};

inline std::string_view BoundedPath(const char* path) noexcept
{
    if (!path) return {};
    std::size_t length = 0;
    while (length <= morgan::fileio::MaxPathBytes && path[length] != '\0') ++length;
    // A one-byte-over-limit view lets the resolver report the actual length rejection.
    return {path, length};
}

inline bool IsSupportedRead(const char* stdioMode, std::uint32_t access) noexcept
{
    return access == 0 && stdioMode && std::strcmp(stdioMode, "rb") == 0;
}
