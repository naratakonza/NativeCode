#pragma once

#include <cstdint>

enum eSoundLoadedState : std::uint8_t
{
    SOUND_NOT_LOADED = 0,
    SOUND_LOADING = 1,
    SOUND_LOADED = 2
};
