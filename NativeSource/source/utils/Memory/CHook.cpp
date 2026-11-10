#include "CHook.h"
#include "../FileIO/FileOpenDiagnostics.h"
#include "../Storage/CStorage.h"

#include <cerrno>
#include <cstring>
#include <mutex>
#include <new>
#include <utility>

static std::shared_ptr<const morgan::fileio::HookAdapter> gameStreamEndpoint;
static GameStreamDiagnostics gameStreamDiagnostics;
static constexpr morgan::fileio::ReplacementOpen activeReplacement{nullptr, morgan::fileio::CStorage::OpenActiveResource};

// Owns the publication to the C entry. In-flight callers retain their own endpoint copy.
struct FileOpenRegistration
{
    explicit FileOpenRegistration(std::shared_ptr<const morgan::fileio::HookAdapter> endpoint) noexcept {
        std::atomic_store(&gameStreamEndpoint, std::move(endpoint));
    }

    ~FileOpenRegistration() {
        std::atomic_store(&gameStreamEndpoint, std::shared_ptr<const morgan::fileio::HookAdapter>{});
    }
};

class HookTransition
{
public:
    explicit HookTransition(std::atomic_flag& flag) noexcept : flag_(flag), acquired_(!flag_.test_and_set()) {}
    ~HookTransition() {
        if (acquired_) flag_.clear();
    }

    bool Acquired() const noexcept { return acquired_; }

private:
    std::atomic_flag& flag_;
    bool acquired_;
};

static std::FILE* OpenOriginal(const char* path, const char* mode, GameStreamDiagnostics* diagnostics) noexcept
{
    if (diagnostics) diagnostics->RecordOriginalCall();
    auto* file = std::fopen(path, mode);
    if (!file && diagnostics) diagnostics->RecordError(StreamError::OriginalOpenFailed);
    return file;
}

struct StreamCall
{
    const char* resolvedPath;
    const char* stdioMode;
    GameStreamDiagnostics* diagnostics;
    std::FILE* stream = nullptr;
    int resultErrno;
};

static morgan::fileio::NativeStatus OpenOriginalStream(void*, const morgan::fileio::NativeOpenRequest& request) noexcept
{
    auto& call = *static_cast<StreamCall*>(request.nativeArguments);
    errno = call.resultErrno;
    call.stream = OpenOriginal(call.resolvedPath, call.stdioMode, call.diagnostics);
    call.resultErrno = errno;
    return {call.stream ? 0 : 1, call.stream != nullptr};
}

static morgan::fileio::NativeStatus AdoptReadStream(void*, const morgan::fileio::NativeOpenRequest& request, morgan::fileio::File stream) noexcept
{
    auto& call = *static_cast<StreamCall*>(request.nativeArguments);
    call.stream = stream.release();
    return {0, true};
}

static morgan::fileio::GameAssetPath ResolveGameStreamPath(const char* logicalPath, const char* resolvedPath,
                                             std::string_view bundleRoot) noexcept
{
    return morgan::fileio::ResolveGameOpenPath(BoundedPath(logicalPath), BoundedPath(resolvedPath), bundleRoot);
}

static std::FILE* OS_FileOpen_hook(const morgan::fileio::HookAdapter* endpoint, const char* resolvedPath,
                           const char* stdioMode, const char* logicalPath, std::uint32_t access,
                           GameStreamDiagnostics* diagnostics, StartupTrace* runtimeTrace,
                           const morgan::fileio::GameAssetPath* classified, bool externalOnly) noexcept
{
    const auto path = BoundedPath(logicalPath);
    const bool strict = externalOnly || (endpoint && endpoint->IsExternalOnly());
    const auto classifiedPath = classified ? *classified
        : (strict || runtimeTrace) ? morgan::fileio::ResolveGameOpenPath(path, {}, {}) : morgan::fileio::GameAssetPath{};
    const auto& asset = classifiedPath.asset;
    const auto record = [&](RuntimeOpenRoute route, bool opened, int error) noexcept {
        if (runtimeTrace) {
            runtimeTrace->RecordRuntimeOpen(path, stdioMode, access, route, opened, error, &classifiedPath,
                                            BoundedPath(resolvedPath));
        }
        errno = error;
    };

    if (!resolvedPath || !stdioMode) {
        if (diagnostics) diagnostics->RecordError(StreamError::InvalidRequest);
        record(RuntimeOpenRoute::BridgeFailed, false, EINVAL);
        return nullptr;
    }

    const bool supportedRead = IsSupportedRead(stdioMode, access);
    const bool endpointActive = endpoint && endpoint->IsRedirectEnabled();
    const bool knownAsset = asset.group < morgan::fileio::storage::GroupCount;
    if (strict && asset.disposition == morgan::fileio::Disposition::Reject) {
        if (diagnostics) diagnostics->RecordError(StreamError::InvalidRequest);
        record(RuntimeOpenRoute::PathRejected, false, EINVAL);
        return nullptr;
    }
    if (strict && knownAsset && !supportedRead) {
        if (diagnostics) diagnostics->RecordError(StreamError::InvalidRequest);
        record(RuntimeOpenRoute::AccessRejected, false, EACCES);
        return nullptr;
    }
    if (strict && knownAsset && !endpointActive) {
        if (diagnostics) diagnostics->RecordError(StreamError::AdapterError);
        record(RuntimeOpenRoute::BridgeFailed, false, EIO);
        return nullptr;
    }
    if (!endpointActive || !supportedRead) {
        auto* stream = OpenOriginal(resolvedPath, stdioMode, diagnostics);
        const int resultErrno = errno;
        record(supportedRead ? RuntimeOpenRoute::OriginalEndpointInactive : RuntimeOpenRoute::OriginalAccess,
               stream != nullptr, resultErrno);
        return stream;
    }
    if (path.empty()) {
        if (diagnostics) diagnostics->RecordError(StreamError::InvalidRequest);
        record(RuntimeOpenRoute::PathRejected, false, EINVAL);
        return nullptr;
    }

    StreamCall call{resolvedPath, stdioMode, diagnostics, nullptr, errno};
    const auto result = endpoint->Open({path, morgan::fileio::Access::ReadOnly, &call, strict ? &asset : nullptr});
    if (diagnostics) diagnostics->RecordRedirect(result);
    if (result.adapterError) {
        const int resultErrno = result.adapterError.category() == std::generic_category() ? result.adapterError.value() : EIO;
        const auto route = result.replacementError ? RuntimeOpenRoute::ExternalFailed : RuntimeOpenRoute::BridgeFailed;
        record(route, false, resultErrno);
        return nullptr;
    }
    if (strict && !knownAsset) {
        record(RuntimeOpenRoute::OriginalOutsidePolicy, call.stream != nullptr, call.resultErrno);
    } else if (strict && call.stream &&
               (classifiedPath.usedResolvedPath || classifiedPath.inputKind == morgan::fileio::AssetPathKind::BundleAbsolute)) {
        record(RuntimeOpenRoute::ExternalOpened, true, call.resultErrno);
    } else {
        errno = call.resultErrno;
    }
    return call.stream;
}

CHook::CHook()
    : adapter_(std::make_shared<morgan::fileio::HookAdapter>(
          morgan::fileio::NativeOpenCallbacks{nullptr, OpenOriginalStream, AdoptReadStream}, activeReplacement))
{
}

CHook::~CHook()
{
    transitioning_.test_and_set();
    state_.store(State::Unavailable);
    adapter_->DisableSamp();
    registration_.reset();
}

CHook* CHook::GetThread()
{
    static CHook hook;
    return &hook;
}

std::error_code CHook::InstallFileOpen() noexcept
{
    if (state_.load() == State::Installed)
    {
        return {};
    }

    const HookTransition transition(transitioning_);
    if (!transition.Acquired())
    {
        return std::make_error_code(std::errc::device_or_resource_busy);
    }

    if (state_.load() == State::Installed)
    {
        return {};
    }

    state_.store(State::Installing);
    try
    {
        registration_ = std::make_shared<FileOpenRegistration>(adapter_);
    }
    catch (const std::bad_alloc&)
    {
        state_.store(State::AdapterReady);
        return std::make_error_code(std::errc::not_enough_memory);
    }

    state_.store(State::Installed);
    return {};
}

std::error_code CHook::EnableSamp(morgan::fileio::Mode mode) noexcept
{
    const HookTransition transition(transitioning_);
    if (!transition.Acquired()) return std::make_error_code(std::errc::device_or_resource_busy);
    if (state_.load() != State::Installed) return std::make_error_code(std::errc::not_connected);
    if (!adapter_->EnableSamp(mode)) return std::make_error_code(std::errc::operation_not_supported);
    return {};
}

std::error_code CHook::DisableSamp() noexcept
{
    const HookTransition transition(transitioning_);
    if (!transition.Acquired()) return std::make_error_code(std::errc::device_or_resource_busy);
    adapter_->DisableSamp();
    return {};
}

std::error_code CHook::SetGameStreamMode(morgan::fileio::Mode mode)
{
    if (mode != morgan::fileio::Mode::OriginalOnly && mode != morgan::fileio::Mode::SampReadOnly && mode != morgan::fileio::Mode::ExternalOnly) {
        return std::make_error_code(std::errc::invalid_argument);
    }

    static std::mutex modeMutex;
    const std::lock_guard<std::mutex> lock(modeMutex);
    if (mode == morgan::fileio::Mode::OriginalOnly) {
        gameStreamDiagnostics.SelectOriginal();
        morgan::fileio::CStorage::UseOriginalAssets();
        return {};
    }
    const auto selection = gameStreamDiagnostics.Selection();
    const bool strict = mode == morgan::fileio::Mode::ExternalOnly;
    const auto desired = strict ? StreamSelection::ExternalOnly : StreamSelection::SampReadOnly;
    if (selection == desired) return {};
    if (selection != StreamSelection::Waiting && (strict || selection != StreamSelection::WaitingAfterAssetRead)) {
        return std::make_error_code(std::errc::operation_not_permitted);
    }

    if (strict && morgan::fileio::CStorage::GameBundleResourcePath().empty()) {
        return std::make_error_code(std::errc::no_such_file_or_directory);
    }

    try {
        if (const auto error = morgan::fileio::CStorage::SelectSampAssets(strict)) return error;
        CHook* hook = CHook::GetThread();
        if (const auto error = hook->InstallFileOpen()) {
            morgan::fileio::CStorage::UseOriginalAssets();
            return error;
        }
        if (const auto error = hook->EnableSamp(mode)) {
            morgan::fileio::CStorage::UseOriginalAssets();
            return error;
        }
        const auto endpoint = std::atomic_load(&gameStreamEndpoint);
        if (!endpoint || !endpoint->IsRedirectEnabled()) {
            hook->DisableSamp();
            morgan::fileio::CStorage::UseOriginalAssets();
            return std::make_error_code(std::errc::io_error);
        }
        if (strict ? gameStreamDiagnostics.SelectExternalOnly() : gameStreamDiagnostics.SelectSamp()) return {};
        hook->DisableSamp();
        morgan::fileio::CStorage::UseOriginalAssets();
        return std::make_error_code(std::errc::operation_not_permitted);
    } catch (const std::bad_alloc&) {
        morgan::fileio::CStorage::UseOriginalAssets();
        return std::make_error_code(std::errc::not_enough_memory);
    }
}

StreamSnapshot CHook::GameStreamSnapshot() noexcept
{
    return gameStreamDiagnostics.Snapshot();
}

extern "C" __attribute__((visibility("default"))) std::FILE* MGOpenGameReadStream(
    const char* resolvedPath, const char* stdioMode, const char* logicalPath, std::uint32_t access) noexcept {
    const int incomingErrno = errno;
    const auto selection = gameStreamDiagnostics.Selection();
    const bool classify = selection != StreamSelection::OriginalOnly &&
        selection != StreamSelection::OriginalOnlyAfterEarlyRead && selection != StreamSelection::SampReadOnly;
    const auto path = classify ? ResolveGameStreamPath(logicalPath, resolvedPath,
        morgan::fileio::CStorage::GameBundleResourcePath()) : morgan::fileio::GameAssetPath{};
    const auto* asset = classify ? &path.asset : nullptr;
    const bool redirectSelected = gameStreamDiagnostics.BeginOpen(stdioMode, logicalPath, access, asset);
    auto endpoint = redirectSelected ? std::atomic_load(&gameStreamEndpoint) : nullptr;
    auto& startup = GameStartupTrace();
    const bool externalOnly = redirectSelected &&
        gameStreamDiagnostics.Selection() == StreamSelection::ExternalOnly;
    const auto read = (!redirectSelected || externalOnly) && access == 0 && stdioMode &&
        std::strcmp(stdioMode, "rb") == 0 ? startup.BeginRead(logicalPath, externalOnly, asset)
                                         : StartupRead{};
    errno = incomingErrno;
    std::FILE* stream = OS_FileOpen_hook(endpoint.get(), resolvedPath, stdioMode,
                                                           logicalPath, access, &gameStreamDiagnostics,
                                                           externalOnly ? &startup : nullptr,
                                                           classify ? &path : nullptr, externalOnly);
    const int resultErrno = errno;
    startup.FinishRead(read, stream != nullptr, resultErrno);
    endpoint.reset();
    errno = resultErrno;
    return stream;
}
