#pragma once

class CTouchInterface
{
public:
    static int IsTouched(int WidgetID, void* pVecOut, int nFrameCount);
    static int IsReleased(int WidgetID, void* pVecOut, int nFrameCount);
    static int IsJustPressed(int WidgetID, void* pVecOut, int nFrameCount);
    static int IsSwipedRight(int WidgetID, int nFrameCount);
    static int IsSwipedLeft(int WidgetID, int nFrameCount);
    static bool IsHeldDown(int WidgetID, int nFrameCount);
    static bool IsPinchZooming(int WidgetID, void* pVecOut, int nFrameCount);
};