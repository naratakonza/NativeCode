#pragma once

#include "FileIO.h"

#include <atomic>
#include <cstdint>
#include <optional>

namespace morgan::fileio {

struct NativeOpenRequest
{
    std::string_view path;
    Access access;
    // The ABI bridge owns this request and any native out-slot it refers to.
    void* nativeArguments = nullptr;
    const Resolution* asset = nullptr;
};

struct NativeStatus
{
    std::int32_t value;
    bool succeeded;
};

// A Morgan-side bridge contract. It is not a declaration of GTA's function ABI.
struct NativeOpenCallbacks
{
    void* context = nullptr;
    NativeStatus (*original)(void*, const NativeOpenRequest&) noexcept = nullptr;
    // Takes ownership of the secure stream. Only the loader can produce its native handle.
    NativeStatus (*adopt)(void*, const NativeOpenRequest&, File) noexcept = nullptr;
};

struct HookResult
{
    std::optional<NativeStatus> nativeStatus;
    std::error_code adapterError;
    Resource redirected = Resource::None;
    std::error_code replacementError{};
    std::size_t group = storage::GroupCount;
    bool usedOriginalFallback = false;
    StreamOrigin origin = StreamOrigin::Unspecified;
};

enum class HookEvent {
    OriginalAvailable, RedirectEnabled, RedirectMatch, OriginalFallback, OpenFailure, RedirectDisabled
};

struct HookDiagnostics
{
    void* context = nullptr;
    void (*emit)(void*, HookEvent) noexcept = nullptr;
};

class HookAdapter
{
public:
    // Callback contexts must stay alive and support concurrent calls when shared.
    HookAdapter(NativeOpenCallbacks callbacks, ReplacementOpen replacement, HookDiagnostics diagnostics = {}) noexcept;

    bool IsRedirectEnabled() const noexcept { return mode_.load() != Mode::OriginalOnly; }
    bool IsExternalOnly() const noexcept { return mode_.load() == Mode::ExternalOnly; }
    bool EnableSamp(Mode mode = Mode::SampReadOnly) noexcept;
    void DisableSamp() noexcept;
    HookResult Open(const NativeOpenRequest& request) const noexcept;

private:
    void Emit(HookEvent event) const noexcept;
    HookResult CallOriginal(const NativeOpenRequest& request) const noexcept;

    NativeOpenCallbacks callbacks_;
    ReplacementOpen replacement_;
    HookDiagnostics diagnostics_;
    std::atomic<Mode> mode_{Mode::OriginalOnly};
};

} // namespace morgan::fileio
