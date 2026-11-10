#pragma once

namespace morgan::launcher {

// Access is confined to the UIKit main thread by the runtime bridge.
template <typename Implementation>
class StartupTickState {
public:
    bool Install(Implementation original, Implementation deferred) noexcept {
        if (installed_ || !original || !deferred || original == deferred) return false;
        original_ = original;
        deferred_ = deferred;
        installed_ = true;
        return true;
    }

    bool OwnsDeferred(Implementation current) const noexcept {
        return deferred_ && current == deferred_;
    }

    bool Observe(Implementation current, Implementation observer) noexcept {
        if (!OwnsDeferred(current) || !observer || observer == deferred_ || observer == original_) return false;
        deferred_ = nullptr;
        observer_ = observer;
        return true;
    }

    Implementation TakeOriginal(Implementation current) noexcept {
        if (!OwnsDeferred(current) && (!observer_ || current != observer_)) return nullptr;
        deferred_ = nullptr;
        observer_ = nullptr;
        return original_;
    }

private:
    Implementation original_ = nullptr;
    Implementation deferred_ = nullptr;
    Implementation observer_ = nullptr;
    bool installed_ = false;
};

} // namespace morgan::launcher
