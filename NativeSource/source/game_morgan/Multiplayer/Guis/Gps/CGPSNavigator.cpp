#include "../../../../../Main.h"
#include "CGPSNavigator.h"
#include "../../CMultiPlayer.h"
#include "../CGui.h"
#include "../../../CGame.h"
#include "../../../CSprite.h"

CGPSNavigator::CGPSNavigator()
{
    CGPSNavigator::m_bVisible = false;
    CGPSNavigator::m_position = CVector(0.0f, 0.0f, 0.0f);
    CGPSNavigator::m_circle1Time = 0.0f;
    CGPSNavigator::m_circle2Time = 0.0f;
    CGPSNavigator::m_delayTime = 0.0f;
    CGPSNavigator::m_isAnimating = true;
    CGPSNavigator::m_closingAnimationTime = 0.0f;
    CGPSNavigator::m_isClosing = false;
}

CGPSNavigator::~CGPSNavigator()
{
    CGPSNavigator::m_bVisible = false;
    CGPSNavigator::m_position = CVector(0.0f, 0.0f, 0.0f);
}

void CGPSNavigator::Render()
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
            return;
        }

        if (!CGPSNavigator::m_bVisible) {
            return;
        }

        CVector screenPos;
        float distance = 0.0f;
        bool isOffScreen = false;

        if (!CGPSNavigator::CalculateDistanceAndPosition(screenPos, distance, isOffScreen)) {
            return;
        }

        if (CGPSNavigator::HandleClosingAnimation(distance)) {
            return;
        }

        float angle = 0.0f;
        if (isOffScreen)
        {
            if (distance > 40.0f) {
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

        CGPSNavigator::UpdateCircleAnimation();

        float sizeScale = 1.0f;
        float spriteSize = CGPSNavigator::CalculateMarkerSize(distance, sizeScale);

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

        if (!CGPSNavigator::m_isClosing)
        {
            CGPSNavigator::DrawAnimatedCircles(drawList, center, spriteSize);
        }

        CGPSNavigator::DrawMarker(drawList, center, radius, sizeScale);

        if (isOffScreen)
        {
            CGPSNavigator::DrawPointer(drawList, center, radius, angle, sizeScale);
        }

        CGPSNavigator::DrawDistanceText(drawList, center, radius, sizeScale, distance);
    }
}

bool CGPSNavigator::CalculateDistanceAndPosition(CVector& screenPos, float& distance, bool& isOffScreen)
{
    if (!CSprite::CalcScreenCoors(&m_position, &screenPos, nullptr, nullptr, false, false)) {
        return false;
    }

    CVector playerPos;
    if (!CGame::GetPlayer()) {
        return false;
    }

    CGame::GetPlayer()->GetPosition(&playerPos);
    distance = sqrtf(powf(CGPSNavigator::m_position.x - playerPos.x, 2) + powf(CGPSNavigator::m_position.y - playerPos.y, 2) + powf(CGPSNavigator::m_position.z - playerPos.z, 2));

    if (!std::isfinite(distance) || !RsGlobal || RsGlobal->maximumWidth <= 0 || RsGlobal->maximumHeight <= 0 || !std::isfinite(screenPos.x) || !std::isfinite(screenPos.y) || !std::isfinite(screenPos.z)) {
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

bool CGPSNavigator::HandleClosingAnimation(float distance)
{
    const float distanceThreshold = 5.0f;
    bool isPositionValid = (CGPSNavigator::m_position.x != 0.0f || CGPSNavigator::m_position.y != 0.0f || CGPSNavigator::m_position.z != 0.0f);
    if (isPositionValid && distance <= distanceThreshold && !CGPSNavigator::m_isClosing)
    {
        CGPSNavigator::m_isClosing = true;
        CGPSNavigator::m_closingAnimationTime = 0.0f;
    }

    if (CGPSNavigator::m_isClosing)
    {
        const float closingSpeed = 0.05f;
        CGPSNavigator::m_closingAnimationTime += closingSpeed;
        if (CGPSNavigator::m_closingAnimationTime >= 1.0f)
        {
            CGPSNavigator::SetPosition(0.0f, 0.0f, 0.0f);
            CGPSNavigator::SetVisible(false);

            return true;
        }
    }
    return false;
}

void CGPSNavigator::UpdateCircleAnimation()
{
    const float animationSpeed = 0.015f;
    const float delayDuration = 0.5f;
    const float animationDuration1 = 1.0f;
    const float animationDuration2 = 1.2f;

    if (!CGPSNavigator::m_isClosing && CGPSNavigator::m_bVisible)
    {
        if (CGPSNavigator::m_isAnimating)
        {
            CGPSNavigator::m_circle1Time += animationSpeed;
            if (CGPSNavigator::m_circle1Time > animationDuration1)
            {
                CGPSNavigator::m_circle1Time = animationDuration1;
            }

            CGPSNavigator::m_circle2Time = CGPSNavigator::m_circle1Time + 0.3f;
            if (CGPSNavigator::m_circle2Time > animationDuration2)
            {
                CGPSNavigator::m_circle2Time = animationDuration2;
                CGPSNavigator::m_isAnimating = false;
                CGPSNavigator::m_delayTime = 0.0f;
            }
        }
        else
        {
            CGPSNavigator::m_delayTime += animationSpeed;
            if (CGPSNavigator::m_delayTime >= delayDuration)
            {
                CGPSNavigator::m_isAnimating = true;
                CGPSNavigator::m_circle1Time = 0.0f;
                CGPSNavigator::m_circle2Time = 0.0f;
            }
        }
    }
}

float CGPSNavigator::CalculateMarkerSize(float distance, float& sizeScale)
{
    const float baseSize = 18.0f;
    const float minSize = 15.0f;
    const float maxSize = 27.0f;
    const float scaleFactor = 0.8f;
    float spriteSize = baseSize + (distance * scaleFactor);
    spriteSize = std::max(minSize, std::min(maxSize, spriteSize));

    sizeScale = 1.0f;
    if (CGPSNavigator::m_isClosing)
    {
        sizeScale = CNecessary::EaseOutQuad(1.0f - CGPSNavigator::m_closingAnimationTime);
    }

    return spriteSize * sizeScale;
}

void CGPSNavigator::CalculateMarkerPosition(ImVec2& center, float relativeAngle, float radius, int iScreenWidth, int iScreenHeight)
{
    float edgeOffset = 40.0f;
    float absRelativeAngle = fabsf(relativeAngle);
    float normalizedAngle = absRelativeAngle / 3.14159f;
    float topY = edgeOffset + radius;
    float bottomY = iScreenHeight - edgeOffset - radius;
    float centerY = bottomY - (bottomY - topY) * normalizedAngle;

    if (relativeAngle < 0)
    {
        center = ImVec2(edgeOffset + radius, centerY); // ซ้าย
    }
    else
    {
        center = ImVec2(iScreenWidth - edgeOffset - radius, centerY); // ขวา
    }
}

void CGPSNavigator::DrawCompassArc(ImDrawList* drawList, ImVec2 center, float spriteSize, float relativeAngle, float centerY)
{
    const float arcRadius = spriteSize * 2.0f;
    const int numSegments = 32;
    const float arcAngle = 3.14159f / 3.0f;
    ImVec2 arcPoints[numSegments];
    ImVec2 arcCenter;
    float startAngle, endAngle;

    if (relativeAngle < 0)
    {
        arcCenter = ImVec2(center.x, centerY + 8.0f);
        startAngle = 3.14159f / 2.0f;
        endAngle = 3.14159f / 2.0f + arcAngle;
    }
    else
    {
        arcCenter = ImVec2(center.x, centerY + 8.0f);
        startAngle = 0.0f;
        endAngle = arcAngle;
    }

    for (int i = 0; i < numSegments; i++)
    {
        float t = (float)i / (float)(numSegments - 1);
        float angle = startAngle + (endAngle - startAngle) * t;
        arcPoints[i] = ImVec2(arcCenter.x + arcRadius * cosf(angle), arcCenter.y + arcRadius * sinf(angle));
    }

    drawList->AddPolyline(arcPoints, numSegments, IM_COL32(255, 215, 0, 255), false, 2.0f);

    float midAngle = (startAngle + endAngle) / 2.0f;
    ImVec2 triangleCenter(arcCenter.x + arcRadius * cosf(midAngle), arcCenter.y + arcRadius * sinf(midAngle));
    const float triangleSize = 8.0f;
    ImVec2 trianglePoints[3];

    float triangleAngle = relativeAngle;
    if (relativeAngle < 0)
    {
        triangleAngle += 3.14159f;
    }

    trianglePoints[0] = ImVec2(triangleCenter.x + triangleSize * cosf(triangleAngle), triangleCenter.y + triangleSize * sinf(triangleAngle));
    trianglePoints[1] = ImVec2(triangleCenter.x + triangleSize * 0.5f * cosf(triangleAngle + 2.0944f), triangleCenter.y + triangleSize * 0.5f * sinf(triangleAngle + 2.0944f));
    trianglePoints[2] = ImVec2(triangleCenter.x + triangleSize * 0.5f * cosf(triangleAngle - 2.0944f), triangleCenter.y + triangleSize * 0.5f * sinf(triangleAngle - 2.0944f));

    drawList->AddConvexPolyFilled(trianglePoints, 3, IM_COL32(255, 215, 0, 255));
    drawList->AddPolyline(trianglePoints, 3, IM_COL32(255, 195, 0, 255), true, 1.5f);
}

void CGPSNavigator::DrawAnimatedCircles(ImDrawList* drawList, ImVec2 center, float spriteSize)
{
    const float maxRadius = spriteSize * 2.0f;
    const float thickness = 6.0f;
    const int numSegments = 32;
    const float animationDuration1 = 1.0f;
    const float animationDuration2 = 1.2f;

    if (CGPSNavigator::m_isAnimating || CGPSNavigator::m_circle1Time > 0.0f)
    {
        float easedTime1 = CNecessary::EaseOutQuad(CGPSNavigator::m_circle1Time / animationDuration1);
        float animRadius1 = maxRadius * easedTime1;
        float alpha1 = 255.0f * (1.0f - easedTime1);
        ImVec2 animCirclePoints[numSegments];

        for (int i = 0; i < numSegments; i++)
        {
            float angle = (2.0f * 3.14159f / numSegments) * i;
            animCirclePoints[i] = ImVec2(center.x + animRadius1 * cosf(angle), center.y + animRadius1 * sinf(angle));
        }

        drawList->AddConvexPolyFilled(animCirclePoints, numSegments, IM_COL32(255, 215, 0, static_cast<int>(alpha1 * 0.3f)));
        drawList->AddPolyline(animCirclePoints, numSegments, IM_COL32(255, 215, 0, static_cast<int>(alpha1)), true, thickness);
    }

    if (CGPSNavigator::m_isAnimating || CGPSNavigator::m_circle2Time > 0.0f)
    {
        float easedTime2 = CNecessary::EaseOutQuad(CGPSNavigator::m_circle2Time / animationDuration2);
        float animRadius2 = maxRadius * easedTime2;
        float alpha2 = 255.0f * (1.0f - easedTime2);
        ImVec2 animCirclePoints[numSegments];

        for (int i = 0; i < numSegments; i++)
        {
            float angle = (2.0f * 3.14159f / numSegments) * i;
            animCirclePoints[i] = ImVec2(center.x + animRadius2 * cosf(angle), center.y + animRadius2 * sinf(angle));
        }

        drawList->AddConvexPolyFilled(animCirclePoints, numSegments, IM_COL32(255, 195, 0, static_cast<int>(alpha2 * 0.3f)));
        drawList->AddPolyline(animCirclePoints, numSegments, IM_COL32(255, 195, 0, static_cast<int>(alpha2)), true, thickness);
    }
}

void CGPSNavigator::DrawMarker(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale)
{
    ImVec2 diamondPoints[4];
    diamondPoints[0] = ImVec2(center.x, center.y - radius);
    diamondPoints[1] = ImVec2(center.x + radius, center.y);
    diamondPoints[2] = ImVec2(center.x, center.y + radius);
    diamondPoints[3] = ImVec2(center.x - radius, center.y);

    const float outerOffset = 4.0f * sizeScale;
    ImVec2 diamondPointsOuter[4];
    diamondPointsOuter[0] = ImVec2(center.x, center.y - radius - outerOffset);
    diamondPointsOuter[1] = ImVec2(center.x + radius + outerOffset, center.y);
    diamondPointsOuter[2] = ImVec2(center.x, center.y + radius + outerOffset);
    diamondPointsOuter[3] = ImVec2(center.x - radius - outerOffset, center.y);

    drawList->AddConvexPolyFilled(diamondPoints, 4, IM_COL32(255, 215, 0, 255));
    drawList->AddPolyline(diamondPoints, 4, IM_COL32(0, 0, 0, 150), true, 2.5f * sizeScale);
    drawList->AddPolyline(diamondPointsOuter, 4, IM_COL32(255, 215, 0, 255), true, 2.0f * sizeScale);
}

void CGPSNavigator::DrawPointer(ImDrawList* drawList, ImVec2 center, float radius, float angle, float sizeScale)
{
    float pointerOffset = radius * 1.4f + 20.0f * sizeScale;
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

void CGPSNavigator::DrawDistanceText(ImDrawList* drawList, ImVec2 center, float radius, float sizeScale, float distance)
{
    char distanceText[32];
    snprintf(distanceText, sizeof(distanceText), OBFUSCATE("%dm"), static_cast<int>(distance));
    const float fontSize = 24.0f * sizeScale;
    ImVec2 textPos(center.x, center.y - radius - 35.0f * sizeScale);
    ImFont* font = ImGui::GetFont();
    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, distanceText);
    textPos.x -= textSize.x * 0.5f;

    const float outlineOffset = 1.0f;
    drawList->AddText(font, fontSize, ImVec2(textPos.x + outlineOffset, textPos.y), IM_COL32(0, 0, 0, 255), distanceText);
    drawList->AddText(font, fontSize, ImVec2(textPos.x - outlineOffset, textPos.y), IM_COL32(0, 0, 0, 255), distanceText);
    drawList->AddText(font, fontSize, ImVec2(textPos.x, textPos.y + outlineOffset), IM_COL32(0, 0, 0, 255), distanceText);
    drawList->AddText(font, fontSize, ImVec2(textPos.x, textPos.y - outlineOffset), IM_COL32(0, 0, 0, 255), distanceText);
    drawList->AddText(font, fontSize, textPos, IM_COL32(255, 215, 0, 255), distanceText);
}

void CGPSNavigator::SetPosition(float x, float y, float z)
{
    CGPSNavigator::m_position.x = x;
    CGPSNavigator::m_position.y = y;
    CGPSNavigator::m_position.z = z;

    CGPSNavigator::m_isClosing = false;
    CGPSNavigator::m_closingAnimationTime = 0.0f;
    CGPSNavigator::m_isAnimating = true;
    CGPSNavigator::m_circle1Time = 0.0f;
    CGPSNavigator::m_circle2Time = 0.0f;
    CGPSNavigator::m_delayTime = 0.0f;
}

void CGPSNavigator::SetVisible(bool visible)
{
    if (visible)
    {
        CGPSNavigator::m_isClosing = false;
        CGPSNavigator::m_closingAnimationTime = 0.0f;
        CGPSNavigator::m_isAnimating = true;
        CGPSNavigator::m_circle1Time = 0.0f;
        CGPSNavigator::m_circle2Time = 0.0f;
        CGPSNavigator::m_delayTime = 0.0f;
    }

    CGPSNavigator::m_bVisible = visible;
}