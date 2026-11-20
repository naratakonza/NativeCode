#pragma once

class CDebugInfo
{
public:
    CDebugInfo();
    ~CDebugInfo();
    
    void RenderInfo();
    void RenderTitle();
    void RenderAudioStreamTag();
    void SetRenderFramerate(uint32_t bDraw);
    void SetRenderMemory(uint32_t bDraw);

private:
    std::uint32_t m_dwLastFramerateUpdateTick;
    std::uint32_t m_dwLastTitleUpdateTick;
    std::uint32_t m_dwLastMemoryUpdateTick;
    uint32_t m_uiDrawFPS;
    uint32_t m_uiDrawMem;
    char m_szTitle[64]{};
};