#pragma once

class CHud
{
public:
    static int16_t& m_ItemToFlash;
    static int m_iAlpha;

    static void Initialise();
    static void SetAlpha(int Alpha);
    static float GetAlpha();
};