#include "FileOpenDiagnostics.h"

#include <algorithm>
#include <cerrno>

static void CopySafeRelativePath(std::string_view input, StartupAssetPath& output) noexcept
{
    if (input.size() > morgan::fileio::storage::MaxAssetPathBytes) return;
    std::transform(input.begin(), input.end(), output.begin(), [](char c) { return c == '\\' ? '/' : c; });
    if (!morgan::fileio::storage::SafeAssetPath({output.data(), input.size()})) output.fill(0);
}

StartupTrace& GameStartupTrace() noexcept
{
    static StartupTrace trace;
    return trace;
}

void StartupTrace::Note(StartupPhase phase) noexcept
{
    const auto flag = std::uint32_t{1} << static_cast<unsigned>(phase);
    if (!(phases_.fetch_or(flag, std::memory_order_relaxed) & flag)) {
        lastPhase_.store(phase, std::memory_order_relaxed);
    }
}

StartupRead StartupTrace::BeginRead(const char* logicalPath, bool externalOnly,
                                     const morgan::fileio::Resolution* classified) noexcept
{
    if (!externalOnly && (phases_.load(std::memory_order_relaxed) &
        (std::uint32_t{1} << static_cast<unsigned>(StartupPhase::HandoffCompleted)))) return {};
    const auto asset = classified ? *classified : morgan::fileio::ResolveGameAssetPath(BoundedPath(logicalPath));
    if (asset.disposition != morgan::fileio::Disposition::Redirect) return {};

    StartupRead read;
    read.sequence = sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    read.group = asset.group;
    read.external = externalOnly;
    (externalOnly ? runtimeReadAttempts_ : readAttempts_).fetch_add(1, std::memory_order_relaxed);
    auto path = asset.Path();
    if (path.empty()) {
        path = asset.resource == morgan::fileio::Resource::MainScript
            ? "data/script/mainv1.scm" : "data/script/scriptv1.img";
    }
    std::copy(path.begin(), path.end(), read.path.begin());

    const std::unique_lock<std::mutex> guard(lock_, std::try_to_lock);
    if (!guard.owns_lock()) {
        droppedEvents_.fetch_add(1, std::memory_order_relaxed);
    } else if (read.sequence > reads_.lastRead.sequence) {
        reads_.lastRead = read;
        reads_.readResult = StartupReadResult::InFlight;
        reads_.nativeError = 0;
    }
    return read;
}

void StartupTrace::FinishRead(const StartupRead& read, bool opened, int error) noexcept
{
    if (!read.sequence) return;
    const std::unique_lock<std::mutex> guard(lock_, std::try_to_lock);
    if (!guard.owns_lock()) {
        droppedEvents_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (read.sequence >= reads_.lastRead.sequence) {
        reads_.lastRead = read;
        reads_.readResult = opened ? StartupReadResult::Opened
            : error == ENOENT ? StartupReadResult::Missing : StartupReadResult::Failed;
        reads_.nativeError = opened ? 0 : error;
    }
    if (opened && read.external && read.sequence > lastExternalSuccessSequence_) {
        reads_.lastSuccessfulExternalGroup = read.group;
        lastExternalSuccessSequence_ = read.sequence;
    }
    if (!opened && read.sequence > reads_.lastFailure.sequence) {
        reads_.lastFailure = read;
        reads_.lastFailureErrno = error;
    }
    if (!opened && error == ENOENT &&
        std::find(reads_.missingPaths.begin(), reads_.missingPaths.end(), read.path) == reads_.missingPaths.end()) {
        reads_.missingPaths[reads_.missingCount % reads_.missingPaths.size()] = read.path;
        ++reads_.missingCount;
    }
}

void StartupTrace::RecordRuntimeOpen(std::string_view path, const char* mode, std::uint32_t access,
                                     RuntimeOpenRoute route, bool opened, int error,
                                     const morgan::fileio::GameAssetPath* classified, std::string_view resolvedPath) noexcept
{
    const auto resolved = classified ? *classified : morgan::fileio::ResolveGameOpenPath(path, {}, {});
    const auto& asset = resolved.asset;
    auto normalized = asset.Path();
    if (normalized.empty() && asset.resource != morgan::fileio::Resource::None) {
        normalized = asset.resource == morgan::fileio::Resource::MainScript ? "SAMP/main.scm" : "SAMP/script.img";
    }
    auto openMode = RuntimeOpenMode::Other;
    if (mode && std::strcmp(mode, "rb") == 0) openMode = RuntimeOpenMode::Read;
    else if (mode && (std::strcmp(mode, "wb") == 0 || std::strcmp(mode, "ab") == 0)) openMode = RuntimeOpenMode::Write;
    else if (mode && (std::strcmp(mode, "rb+") == 0 || std::strcmp(mode, "wb+") == 0)) openMode = RuntimeOpenMode::Update;

    RuntimeOpenEvent event{route, resolved.inputKind, openMode, access, opened ? 0 : error, opened,
                           asset.group, asset.disposition, resolved.issue, resolved.usedResolvedPath,
                           resolved.resolvedKind, resolved.rejection, resolved.stockTrailingSpace};
    if (asset.group < morgan::fileio::storage::GroupCount) {
        CopySafeRelativePath(normalized, event.path);
    } else if (!opened && (resolved.inputKind == RuntimePathKind::Rootless ||
                          resolved.inputKind == RuntimePathKind::Relative)) {
        // An unclassified filename is evidence, not a redirect candidate.
        if (path.size() > 2 && path[0] == '.' && (path[1] == '/' || path[1] == '\\')) path.remove_prefix(2);
        CopySafeRelativePath(path, event.path);
    }
    if ((!opened || resolved.stockTrailingSpace) && resolved.bundleSuffixOffset &&
        resolved.bundleSuffixOffset < resolvedPath.size()) {
        CopySafeRelativePath(resolvedPath.substr(resolved.bundleSuffixOffset), event.trustedResolvedSuffix);
    }
    {
        const std::unique_lock<std::mutex> guard(lock_, std::try_to_lock);
        if (guard.owns_lock()) {
            reads_.runtimeOpenEvents[reads_.runtimeOpenEventCount % reads_.runtimeOpenEvents.size()] = event;
            ++reads_.runtimeOpenEventCount;
        } else {
            droppedEvents_.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (!opened) {
        if (const auto sink = failureSink_.load()) sink(event);
    }
}

StartupSnapshot StartupTrace::Snapshot() noexcept
{
    StartupSnapshot result;
    {
        const std::lock_guard<std::mutex> guard(lock_);
        result = reads_;
    }
    result.lastPhase = lastPhase_.load(std::memory_order_relaxed);
    result.phases = phases_.load(std::memory_order_relaxed);
    result.readAttempts = readAttempts_.load(std::memory_order_relaxed);
    result.runtimeReadAttempts = runtimeReadAttempts_.load(std::memory_order_relaxed);
    result.droppedEvents = droppedEvents_.load(std::memory_order_relaxed);
    return result;
}

bool GameStreamDiagnostics::BeginOpen(const char* stdioMode, const char* logicalPath, std::uint32_t access,
                                      const morgan::fileio::Resolution* classified) noexcept
{
    bridgeEntryCalls_.fetch_add(1, std::memory_order_relaxed);
    auto selected = selection_.load();
    if ((selected == StreamSelection::Waiting || selected == StreamSelection::WaitingAfterAssetRead ||
         selected == StreamSelection::EarlyRead) && IsSupportedRead(stdioMode, access)) {
        const auto asset = classified ? *classified : morgan::fileio::ResolveGameAssetPath(BoundedPath(logicalPath));
        if (asset.disposition == morgan::fileio::Disposition::Redirect) {
            const auto next = asset.resource == morgan::fileio::Resource::None
                ? StreamSelection::WaitingAfterAssetRead : StreamSelection::EarlyRead;
            while (selected == StreamSelection::Waiting || selected == StreamSelection::WaitingAfterAssetRead ||
                   selected == StreamSelection::EarlyRead) {
                if (selected == StreamSelection::EarlyRead || selection_.compare_exchange_weak(selected, next)) {
                    groups_[asset.group].earlyReads.fetch_add(1, std::memory_order_relaxed);
                    return false;
                }
            }
        } else {
            selected = selection_.load();
        }
    }
    return selected == StreamSelection::SampReadOnly || selected == StreamSelection::ExternalOnly;
}

bool GameStreamDiagnostics::SelectSamp() noexcept
{
    auto expected = selection_.load();
    while (expected == StreamSelection::Waiting || expected == StreamSelection::WaitingAfterAssetRead) {
        if (selection_.compare_exchange_weak(expected, StreamSelection::SampReadOnly)) return true;
    }
    return expected == StreamSelection::SampReadOnly;
}

bool GameStreamDiagnostics::SelectExternalOnly() noexcept
{
    auto expected = StreamSelection::Waiting;
    return selection_.compare_exchange_strong(expected, StreamSelection::ExternalOnly) ||
           expected == StreamSelection::ExternalOnly;
}

void GameStreamDiagnostics::SelectOriginal() noexcept
{
    auto previous = selection_.load();
    do {
        const auto selected = (previous == StreamSelection::EarlyRead ||
                               previous == StreamSelection::OriginalOnlyAfterEarlyRead)
            ? StreamSelection::OriginalOnlyAfterEarlyRead : StreamSelection::OriginalOnly;
        if (selection_.compare_exchange_weak(previous, selected)) return;
    } while (true);
}

void GameStreamDiagnostics::RecordOriginalCall() noexcept
{
    originalFopenCalls_.fetch_add(1, std::memory_order_relaxed);
}

void GameStreamDiagnostics::RecordError(StreamError error) noexcept
{
    lastError_.store(error, std::memory_order_relaxed);
}

void GameStreamDiagnostics::RecordRedirect(const morgan::fileio::HookResult& result) noexcept
{
    if (result.group < groups_.size()) {
        auto& group = groups_[result.group];
        group.attempts.fetch_add(1, std::memory_order_relaxed);
        if (result.replacementError || result.adapterError || !result.nativeStatus || !result.nativeStatus->succeeded) {
            group.failures.fetch_add(1, std::memory_order_relaxed);
        } else {
            group.successes.fetch_add(1, std::memory_order_relaxed);
            if (result.origin == morgan::fileio::StreamOrigin::External) {
                group.externalSuccesses.fetch_add(1, std::memory_order_relaxed);
            } else if (result.origin == morgan::fileio::StreamOrigin::Bundle) {
                group.bundleSuccesses.fetch_add(1, std::memory_order_relaxed);
            }
        }
        if (result.usedOriginalFallback) group.fallbacks.fetch_add(1, std::memory_order_relaxed);
    }
    if (result.redirected == morgan::fileio::Resource::MainScript) {
        mainScriptRedirects_.fetch_add(1, std::memory_order_relaxed);
    } else if (result.redirected == morgan::fileio::Resource::ScriptImage) {
        scriptImageRedirects_.fetch_add(1, std::memory_order_relaxed);
    }
    if (result.replacementError) {
        redirectFailures_.fetch_add(1, std::memory_order_relaxed);
        if (result.replacementError != std::errc::no_such_file_or_directory) {
            RecordError(StreamError::ReplacementRejected);
        } else if (!result.usedOriginalFallback || (result.nativeStatus && result.nativeStatus->succeeded)) {
            RecordError(StreamError::MissingReplacement);
        }
    } else if (result.adapterError) {
        if (result.adapterError == std::errc::invalid_argument) {
            RecordError(StreamError::InvalidRequest);
        } else if (result.adapterError == std::errc::resource_deadlock_would_occur) {
            RecordError(StreamError::RecursiveOpen);
        } else {
            RecordError(StreamError::AdapterError);
        }
    }
}

StreamSnapshot GameStreamDiagnostics::Snapshot() const noexcept
{
    StreamSnapshot snapshot{bridgeEntryCalls_.load(std::memory_order_relaxed), originalFopenCalls_.load(std::memory_order_relaxed),
                            mainScriptRedirects_.load(std::memory_order_relaxed), scriptImageRedirects_.load(std::memory_order_relaxed),
                            redirectFailures_.load(std::memory_order_relaxed), lastError_.load(std::memory_order_relaxed),
                            selection_.load()};
    for (std::size_t i = 0; i < groups_.size(); ++i) {
        const auto& group = groups_[i];
        snapshot.groups[i] = {group.attempts.load(std::memory_order_relaxed), group.successes.load(std::memory_order_relaxed),
                              group.failures.load(std::memory_order_relaxed), group.fallbacks.load(std::memory_order_relaxed),
                              group.earlyReads.load(std::memory_order_relaxed),
                              group.externalSuccesses.load(std::memory_order_relaxed),
                              group.bundleSuccesses.load(std::memory_order_relaxed)};
    }
    return snapshot;
}
