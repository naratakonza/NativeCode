#pragma once

class CSphere
{
public:
    struct { float x, y, z; } m_vecCenter{};
    float m_fRadius{};
};
static_assert(sizeof(CSphere) == 0x10);