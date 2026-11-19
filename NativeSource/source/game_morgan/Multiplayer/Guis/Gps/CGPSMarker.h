#pragma once

#include "../../../Core/CVector.h"
#include "../../../../vendor/imgui/imgui.h"

class CGPSMarker
{
private:
    CVector m_position;
    bool m_bVisible;
    float m_animationTime1;
    float m_animationTime2;
    float m_animationTime3;
    float m_delayTime;
    bool m_isAnimating;
    float m_closingAnimationTime;
    bool m_isClosing;

public:
    CGPSMarker();
    ~CGPSMarker();

    void Render();
    void SetPosition(float x, float y, float z);
    void SetVisible(bool visible);

private:
    static float EaseOutQuad(float t);
    bool CalculateDistanceAndPosition(CVector& screenPos, float& distance, bool& isOffScreen);
    bool HandleClosingAnimation(float distance);
    void UpdateHexagonAnimation();
    float CalculateMarkerSize(float distance, float& sizeScale);
    void DrawAnimatedHexagons(ImDrawList* drawList, ImVec2 center, float spriteSize);
    void DrawMarker(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale);
    void DrawPointer(ImDrawList* drawList, ImVec2 center, float radius, float angle, float sizeScale);
    void DrawDistanceText(ImDrawList* drawList, ImVec2 center, float sizeScale, float distance);
};