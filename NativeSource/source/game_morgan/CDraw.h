#pragma once

class CDraw
{
public:
    static float& ms_fFOV;
    static const float& ms_fAspectRatio;
    static float ms_SavefFOV;

    static void Initialise();
    static void SetFovHook(float value, bool isCinematic);
    static void SetFov(float value);
    static float GetFov();
};