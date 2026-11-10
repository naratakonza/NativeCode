#pragma once

#include "../FileIO/HookAdapter.h"

#include <atomic>
#include <memory>
#include <system_error>

struct StreamSnapshot;

class CHook
{
public:
    static CHook* GetThread();
    std::error_code InstallFileOpen() noexcept;

    static std::error_code SetGameStreamMode(morgan::fileio::Mode mode);
    static StreamSnapshot GameStreamSnapshot() noexcept;

private:
    enum class State { Unavailable, AdapterReady, Installing, Installed };

    CHook();
    ~CHook();

    std::error_code EnableSamp(morgan::fileio::Mode mode) noexcept;
    std::error_code DisableSamp() noexcept;

    std::shared_ptr<morgan::fileio::HookAdapter> adapter_;
    std::shared_ptr<void> registration_;
    std::atomic<State> state_{State::AdapterReady};
    std::atomic_flag transitioning_ = ATOMIC_FLAG_INIT;
};