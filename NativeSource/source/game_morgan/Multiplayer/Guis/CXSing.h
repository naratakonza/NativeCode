#pragma once
#include "../../RenderWare/RenderWare.h"

class CXSing
{
public:
    CXSing();
    ~CXSing();

    void Render();
    void Enable(bool status);
    void LoadTexture();

    bool IsKeyXSingActive();
    bool IsXSingActive();
    RwTexture* p_XSingButtonTexture;
    RwTexture* p_XSingHoverButtonTexture;

public:
    bool m_bKeysStatus;
    bool m_bRenderStatus;
};