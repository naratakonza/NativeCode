#pragma once

#include "../../../Core/CVector.h"
#include "../../../../vendor/imgui/imgui.h"

class CGPSNavigator
{
private:
    CVector m_position;
    bool m_bVisible;
    float m_circle1Time;
    float m_circle2Time;
    float m_delayTime;
    bool m_isAnimating;
    float m_closingAnimationTime;
    bool m_isClosing;

public:
    CGPSNavigator();
    ~CGPSNavigator();

    void Render();
    void SetPosition(float x, float y, float z);
    void SetVisible(bool visible);

private:
    bool CalculateDistanceAndPosition(CVector& screenPos, float& distance, bool& isOffScreen);
    bool HandleClosingAnimation(float distance);
    void UpdateCircleAnimation();
    float CalculateMarkerSize(float distance, float& sizeScale);
    void CalculateMarkerPosition(ImVec2& center, float relativeAngle, float radius, int iScreenWidth, int iScreenHeight);
    void DrawCompassArc(ImDrawList* drawList, ImVec2 center, float spriteSize, float relativeAngle, float centerY);
    void DrawAnimatedCircles(ImDrawList* drawList, ImVec2 center, float spriteSize);
    void DrawMarker(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale);
    void DrawPointer(ImDrawList* drawList, ImVec2 center, float radius, float angle, float sizeScale);
    void DrawDistanceText(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale, float distance);
};