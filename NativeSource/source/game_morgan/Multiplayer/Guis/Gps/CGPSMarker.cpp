#include "../../../../../Main.h"
#include "CGPSMarker.h"
#include "../CGui.h"
#include "../../../CGame.h"
#include "../../../CSprite.h"
#include "../../../CRadar.h"
#include "../../CMultiPlayer.h"
#include "../../CNetGame.h"

#include <cfloat>

CGPSMarker::CGPSMarker()
{
    CGPSMarker::m_bVisible = false;
    CGPSMarker::m_position = CVector(0.0f, 0.0f, 0.0f);
    CGPSMarker::m_animationTime1 = 0.0f;
    CGPSMarker::m_animationTime2 = 0.0f;
    CGPSMarker::m_animationTime3 = 0.0f;
    CGPSMarker::m_delayTime = 0.0f;
    CGPSMarker::m_isAnimating = true;
    CGPSMarker::m_closingAnimationTime = 0.0f;
    CGPSMarker::m_isClosing = false;
}

CGPSMarker::~CGPSMarker()
{
    CGPSMarker::m_bVisible = false;
    CGPSMarker::m_position = CVector(0.0f, 0.0f, 0.0f);
}

void CGPSMarker::Render()
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
            return;
        }

        if (!CGPSMarker::m_bVisible) {
            return;
        }

        CVector screenPos;
        float distance = 0.0f;
        bool isOffScreen = false;

        if (!CGPSMarker::CalculateDistanceAndPosition(screenPos, distance, isOffScreen)) {
            return;
        }

        if (CGPSMarker::HandleClosingAnimation(distance)) {
            return;
        }

        float angle = 0.0f;
        if (isOffScreen)
        {
            if (distance > 120.0f) {
                return;
            }

            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            float screenWidth = displaySize.x;
            float screenHeight = displaySize.y;
            float centerX = screenWidth / 2.0f;
            float centerY = screenHeight / 2.0f;

            angle = atan2f(screenPos.y - centerY, screenPos.x - centerX);

            if (screenPos.z < 1.0f)
            {
                angle += 3.1415926535f;
            }

            float paddingX = 50.0f;
            float paddingY = 50.0f;
            float radiusX = (screenWidth / 2.0f) - paddingX;
            float radiusY = (screenHeight / 2.0f) - paddingY;

            screenPos.x = centerX + (cosf(angle) * radiusX);
            screenPos.y = centerY + (sinf(angle) * radiusY);
        }

        CGPSMarker::UpdateHexagonAnimation();

        float sizeScale = 1.0f;
        float spriteSize = CGPSMarker::CalculateMarkerSize(distance, sizeScale);

        if (isOffScreen)
        {
            spriteSize *= 0.7f;
            sizeScale *= 0.7f;
        }

        float radius = spriteSize * 0.5f;

        ImDrawList* drawList = ImGui::GetOverlayDrawList();
        if (!drawList) {
            return;
        }

        ImVec2 center(screenPos.x, screenPos.y);
        CGPSMarker::DrawAnimatedHexagons(drawList, center, spriteSize);

        CGPSMarker::DrawMarker(drawList, center, radius, sizeScale);

        if (isOffScreen)
        {
            CGPSMarker::DrawPointer(drawList, center, radius, angle, sizeScale);
        }

        CGPSMarker::DrawDistanceText(drawList, center, sizeScale, distance);
    }
}

bool CGPSMarker::CalculateDistanceAndPosition(CVector& screenPos, float& distance, bool& isOffScreen)
{
    if (!CGame::GetPlayer()) {
        return false;
    }

    CVector playerPos;
    CGame::GetPlayer()->GetPosition(&playerPos);
    distance = sqrtf((CGPSMarker::m_position.x - playerPos.x) * (CGPSMarker::m_position.x - playerPos.x) + (CGPSMarker::m_position.y - playerPos.y) * (CGPSMarker::m_position.y - playerPos.y) + (CGPSMarker::m_position.z - playerPos.z) * (CGPSMarker::m_position.z - playerPos.z));

    if (!std::isfinite(distance) || !RsGlobal || RsGlobal->maximumWidth <= 0 || RsGlobal->maximumHeight <= 0 ||
        !CSprite::CalcScreenCoors(&this->m_position, &screenPos, nullptr, nullptr, false, false) ||
        !std::isfinite(screenPos.x) || !std::isfinite(screenPos.y) || !std::isfinite(screenPos.z))
    {
        return false;
    }

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    screenPos.x *= displaySize.x / static_cast<float>(RsGlobal->maximumWidth);
    screenPos.y *= displaySize.y / static_cast<float>(RsGlobal->maximumHeight);
    float screenWidth = displaySize.x;
    float screenHeight = displaySize.y;

    bool bIsOnScreen = (screenPos.z >= 1.0f && screenPos.x >= 0.0f && screenPos.x <= screenWidth && screenPos.y >= 0.0f && screenPos.y <= screenHeight);

    isOffScreen = !bIsOnScreen;

    return true;
}

bool CGPSMarker::HandleClosingAnimation(float distance)
{
    const float distanceThreshold = (CGPSMarker::m_position.z > 20.0f) ? 20.0f : 5.0f;
    bool isPositionValid = (CGPSMarker::m_position.x != 0.0f || CGPSMarker::m_position.y != 0.0f || CGPSMarker::m_position.z != 0.0f);

    if (isPositionValid && distance <= distanceThreshold && !CGPSMarker::m_isClosing)
    {
        CGPSMarker::m_isClosing = true;
        CGPSMarker::m_closingAnimationTime = 0.0f;
    }

    if (CGPSMarker::m_isClosing)
    {
        const float closingSpeed = 0.05f;
        CGPSMarker::m_closingAnimationTime += closingSpeed;
        if (CGPSMarker::m_closingAnimationTime >= 1.0f)
        {
            CRadar::ClearWaypoint();
            
            CGPSMarker::SetPosition(0.0f, 0.0f, 0.0f);
            CGPSMarker::SetVisible(false);

            // if (CGui::GetGPSLine())
            // {
            //     CGui::GetGPSLine()->Delete();
            // }

            // if (CJava::GetJava)
            // {
            //     CJava::GetJava()->ShowNotification(OBFUSCATE("The destination has been reached.!"), 10000);
            // }

            return true;
        }
    }

    return false;
}

void CGPSMarker::UpdateHexagonAnimation()
{
    const float animationSpeed = 0.015f;
    const float delayDuration = 0.5f;

    if (!CGPSMarker::m_isClosing)
    {
        if (CGPSMarker::m_isAnimating)
        {
            CGPSMarker::m_animationTime1 += animationSpeed;
            if (CGPSMarker::m_animationTime1 > 1.0f)
            {
                CGPSMarker::m_animationTime1 = 1.0f;
                CGPSMarker::m_isAnimating = false;
                CGPSMarker::m_delayTime = 0.0f;
            }

            CGPSMarker::m_animationTime2 = CGPSMarker::m_animationTime1 + 0.3f;
            if (CGPSMarker::m_animationTime2 > 1.0f)
            {
                CGPSMarker::m_animationTime2 = 1.0f;
            }

            CGPSMarker::m_animationTime3 = CGPSMarker::m_animationTime1 + 0.6f;
            if (CGPSMarker::m_animationTime3 > 1.0f)
            {
                CGPSMarker::m_animationTime3 = 1.0f;
            }
        }
        else
        {
            CGPSMarker::m_delayTime += animationSpeed;
            if (CGPSMarker::m_delayTime >= delayDuration)
            {
                CGPSMarker::m_isAnimating = true;
                CGPSMarker::m_animationTime1 = 0.0f;
                CGPSMarker::m_animationTime2 = 0.0f;
                CGPSMarker::m_animationTime3 = 0.0f;
            }
        }
    }
}

float CGPSMarker::EaseOutQuad(float t)
{
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float CGPSMarker::CalculateMarkerSize(float distance, float& sizeScale)
{
    const float baseSize = 48.0f;
    const float minSize = 40.0f;
    const float maxSize = 72.0f;
    const float scaleFactor = 0.8f;
    float spriteSize = baseSize + (distance * scaleFactor);
    spriteSize = std::max(minSize, std::min(maxSize, spriteSize));

    sizeScale = 1.0f;
    if (CGPSMarker::m_isClosing)
    {
        sizeScale = CGPSMarker::EaseOutQuad(1.0f - CGPSMarker::m_closingAnimationTime);
    }

    return spriteSize * sizeScale;
}

void CGPSMarker::DrawAnimatedHexagons(ImDrawList* drawList, ImVec2 center, float spriteSize)
{
    const float maxRadius = spriteSize * 1.2f;
    const float thickness = 6.0f;

    float easedTime1 = CGPSMarker::EaseOutQuad(CGPSMarker::m_animationTime1);
    float easedTime2 = CGPSMarker::EaseOutQuad(CGPSMarker::m_animationTime2);
    float easedTime3 = CGPSMarker::EaseOutQuad(CGPSMarker::m_animationTime3);

    if (!CGPSMarker::m_isClosing)
    {
        float animRadius1 = maxRadius * easedTime1;
        float alpha1 = 255.0f * (1.0f - easedTime1);
        if (CGPSMarker::m_isAnimating || CGPSMarker::m_animationTime1 > 0.0f)
        {
            ImVec2 animHexagonPoints1[6];
            for (int i = 0; i < 6; i++)
            {
                float angle = 3.14159f / 3.0f * i + 3.14159f / 6.0f;
                animHexagonPoints1[i] = ImVec2(center.x + animRadius1 * cosf(angle), center.y + animRadius1 * sinf(angle));
            }

            drawList->AddConvexPolyFilled(animHexagonPoints1, 6, IM_COL32(200, 200, 200, static_cast<int>(alpha1 * 0.3f)));
            drawList->AddPolyline(animHexagonPoints1, 6, IM_COL32(255, 255, 255, static_cast<int>(alpha1)), true, thickness);
        }

        float animRadius2 = maxRadius * easedTime2;
        float alpha2 = 255.0f * (1.0f - easedTime2);
        if (CGPSMarker::m_isAnimating || CGPSMarker::m_animationTime2 > 0.0f)
        {
            ImVec2 animHexagonPoints2[6];
            for (int i = 0; i < 6; i++)
            {
                float angle = 3.14159f / 3.0f * i + 3.14159f / 6.0f;
                animHexagonPoints2[i] = ImVec2(center.x + animRadius2 * cosf(angle), center.y + animRadius2 * sinf(angle));
            }

            drawList->AddConvexPolyFilled(animHexagonPoints2, 6, IM_COL32(200, 200, 200, static_cast<int>(alpha2 * 0.3f)));
            drawList->AddPolyline(animHexagonPoints2, 6, IM_COL32(200, 200, 200, static_cast<int>(alpha2)), true, thickness);
        }

        float animRadius3 = maxRadius * easedTime3;
        float alpha3 = 255.0f * (1.0f - easedTime3);
        if (CGPSMarker::m_isAnimating || CGPSMarker::m_animationTime3 > 0.0f)
        {
            ImVec2 animHexagonPoints3[6];
            for (int i = 0; i < 6; i++)
            {
                float angle = 3.14159f / 3.0f * i + 3.14159f / 6.0f;
                animHexagonPoints3[i] = ImVec2(center.x + animRadius3 * cosf(angle), center.y + animRadius3 * sinf(angle));
            }

            drawList->AddConvexPolyFilled(animHexagonPoints3, 6, IM_COL32(200, 200, 200, static_cast<int>(alpha3 * 0.3f)));
            drawList->AddPolyline(animHexagonPoints3, 6, IM_COL32(150, 150, 150, static_cast<int>(alpha3)), true, thickness);
        }
    }
}

void CGPSMarker::DrawMarker(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale)
{
    ImVec2 hexagonPoints[6];
    ImVec2 hexagonPointsShadow[6];
    const float shadowOffset = 2.0f;

    for (int i = 0; i < 6; i++)
    {
        float angle = 3.14159f / 3.0f * i + 3.14159f / 6.0f;
        hexagonPoints[i] = ImVec2(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
        hexagonPointsShadow[i] = ImVec2(center.x + radius * cosf(angle) + shadowOffset * sizeScale, center.y + radius * sinf(angle) + shadowOffset * sizeScale);
    }

    drawList->AddConvexPolyFilled(hexagonPoints, 6, IM_COL32(0, 0, 0, 200));
    drawList->AddPolyline(hexagonPointsShadow, 6, IM_COL32(50, 50, 50, 200), true, 3.0f * sizeScale);
    drawList->AddPolyline(hexagonPoints, 6, IM_COL32(255, 215, 0, 255), true, 3.0f * sizeScale);
}

void CGPSMarker::DrawPointer(ImDrawList* drawList, ImVec2 center, float radius, float angle, float sizeScale)
{
    float pointerOffset = radius * 1.4f;
    float arrowLength = 22.0f * sizeScale;
    float arrowWidth = 10.0f * sizeScale;

    ImVec2 tip(center.x + (pointerOffset + arrowLength) * cosf(angle), center.y + (pointerOffset + arrowLength) * sinf(angle));

    float baseCenterX = center.x + pointerOffset * cosf(angle);
    float baseCenterY = center.y + pointerOffset * sinf(angle);

    ImVec2 left(baseCenterX - arrowWidth * sinf(angle), baseCenterY + arrowWidth * cosf(angle));

    ImVec2 right(baseCenterX + arrowWidth * sinf(angle), baseCenterY - arrowWidth * cosf(angle));

    const float shadowOffset = 2.0f;
    ImVec2 shadowTip(tip.x + shadowOffset * sizeScale, tip.y + shadowOffset * sizeScale);
    ImVec2 shadowLeft(left.x + shadowOffset * sizeScale, left.y + shadowOffset * sizeScale);
    ImVec2 shadowRight(right.x + shadowOffset * sizeScale, right.y + shadowOffset * sizeScale);

    drawList->AddTriangleFilled(tip, left, right, IM_COL32(0, 0, 0, 200));
    drawList->AddTriangle(shadowTip, shadowLeft, shadowRight, IM_COL32(50, 50, 50, 200), 3.0f * sizeScale);
    drawList->AddTriangle(tip, left, right, IM_COL32(255, 215, 0, 255), 3.0f * sizeScale);
}

void CGPSMarker::DrawDistanceText(ImDrawList* drawList, ImVec2 center, float sizeScale, float distance)
{
    char distanceText[32];
    std::snprintf(distanceText, sizeof(distanceText), OBFUSCATE("%.0fm"), std::trunc(distance));
    const float fontSize = 24.0f * sizeScale;
    ImVec2 textPos(center.x, center.y);

    ImFont* font = ImGui::GetFont();
    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, distanceText);
    textPos.x -= textSize.x * 0.5f;
    textPos.y -= textSize.y * 0.5f;

    drawList->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), distanceText);
}

void CGPSMarker::SetPosition(float x, float y, float z)
{
    CGPSMarker::m_position.x = x;
    CGPSMarker::m_position.y = y;
    CGPSMarker::m_position.z = z;

    CGPSMarker::m_isClosing = false;
    CGPSMarker::m_closingAnimationTime = 0.0f;
    CGPSMarker::m_isAnimating = true;
    CGPSMarker::m_animationTime1 = 0.0f;
    CGPSMarker::m_animationTime2 = 0.0f;
    CGPSMarker::m_animationTime3 = 0.0f;
    CGPSMarker::m_delayTime = 0.0f;
}

void CGPSMarker::SetVisible(bool visible)
{
    CGPSMarker::m_bVisible = visible;
}
