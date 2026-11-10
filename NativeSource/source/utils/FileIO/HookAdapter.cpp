#include "HookAdapter.h"

#include <utility>

namespace morgan::fileio {
namespace {

thread_local bool opening = false;

struct OpenScope
{
    OpenScope() noexcept { opening = true; }
    ~OpenScope() { opening = false; }
};

} // namespace

HookAdapter::HookAdapter(NativeOpenCallbacks callbacks, ReplacementOpen replacement, HookDiagnostics diagnostics) noexcept
    : callbacks_(callbacks), replacement_(replacement), diagnostics_(diagnostics) {
    if (callbacks_.original) Emit(HookEvent::OriginalAvailable);
}

bool HookAdapter::EnableSamp(Mode mode) noexcept
{
    if (mode == Mode::OriginalOnly || !callbacks_.original || !callbacks_.adopt || !replacement_.call) return false;
    if (mode_.exchange(mode) == Mode::OriginalOnly) Emit(HookEvent::RedirectEnabled);
    return true;
}

void HookAdapter::DisableSamp() noexcept
{
    if (mode_.exchange(Mode::OriginalOnly) != Mode::OriginalOnly) Emit(HookEvent::RedirectDisabled);
}

void HookAdapter::Emit(HookEvent event) const noexcept
{
#ifndef NDEBUG
    if (diagnostics_.emit) diagnostics_.emit(diagnostics_.context, event);
#else
    (void)diagnostics_;
    (void)event;
#endif
}

HookResult HookAdapter::CallOriginal(const NativeOpenRequest& request) const noexcept
{
    const auto status = callbacks_.original(callbacks_.context, request);
    if (!status.succeeded) Emit(HookEvent::OpenFailure);
    return {status, {}};
}

HookResult HookAdapter::Open(const NativeOpenRequest& request) const noexcept
{
    if (opening) return {{}, std::make_error_code(std::errc::resource_deadlock_would_occur)};
    const OpenScope scope;
    if (!callbacks_.original) return {{}, std::make_error_code(std::errc::operation_not_supported)};
    const auto mode = mode_.load();
    // A supplied resolution belongs to a call admitted as ExternalOnly.
    if (request.asset && mode != Mode::ExternalOnly) {
        return {{}, std::make_error_code(std::errc::io_error)};
    }
    if (mode == Mode::OriginalOnly || request.access != Access::ReadOnly) {
        return CallOriginal(request);
    }

    const auto resolution = request.asset ? *request.asset : ResolveGameAssetPath(request.path);
    if (resolution.disposition == Disposition::Original) {
        return CallOriginal(request);
    }
    if (resolution.disposition == Disposition::Reject) {
        Emit(HookEvent::OpenFailure);
        return {{}, std::make_error_code(std::errc::invalid_argument)};
    }

    Emit(HookEvent::RedirectMatch);
    auto opened = replacement_.call(replacement_.context, resolution);
    if (mode == Mode::ExternalOnly && opened.origin == StreamOrigin::Bundle) {
        Emit(HookEvent::OpenFailure);
        const auto error = std::make_error_code(std::errc::permission_denied);
        return {{}, error, Resource::None, error, resolution.group};
    }
    if (opened.file && !opened.error) {
        const auto status = callbacks_.adopt(callbacks_.context, request, std::move(opened.file));
        if (status.succeeded) return {status, {}, resolution.resource, {}, resolution.group, false, opened.origin};
        Emit(HookEvent::OpenFailure);
        return {status, {}, Resource::None, std::make_error_code(std::errc::io_error), resolution.group};
    }
    if (mode == Mode::SampReadOnly && !opened.file && opened.error == std::errc::no_such_file_or_directory) {
        Emit(HookEvent::OriginalFallback);
        auto fallback = CallOriginal(request);
        fallback.replacementError = opened.error;
        fallback.group = resolution.group;
        fallback.usedOriginalFallback = true;
        return fallback;
    }

    Emit(HookEvent::OpenFailure);
    const auto error = opened.error ? opened.error : std::make_error_code(std::errc::io_error);
    return {{}, error, Resource::None, error, resolution.group};
}

} // namespace morgan::fileio
