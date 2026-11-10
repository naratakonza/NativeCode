#pragma once

class CDebugInfo
{
public:
    CDebugInfo();

    void RenderTitle();

private:
    std::uint32_t m_dwLastTitleUpdateTick;
    char m_szTitle[64]{};
};
