#pragma once

class CTiltedRadar
{
private:
    static bool m_bEnabled;
    static float m_fCenterX;
    static float m_fCenterY;
    static float m_fRadiusX;
    static float m_fRadiusY;
    static bool m_bBoundsValid;

    static void UpdateRadarEdgeTable(float centerX, float centerY, float radiusX, float radiusY);
    static void DrawRadarBackgroundDisk(float centerX, float centerY, float radiusX, float radiusY, int alpha);
    static void Render3DMapTile(int sectionX, int sectionY, int alpha, float centerX, float centerY, float radiusX, float radiusY, float rtSize, float aspect, float playerWorldX, float playerWorldY);
    static bool ProjectWorldToTiltedHud(float worldX, float worldY, float& outX, float& outY);
    static void SetBounds(float cx, float cy, float rx, float ry);
    static bool UpdateRadarWidgetBounds();

public:
    static void Initialise();
    static void SetEnabled(bool bEnabled);
    static bool IsEnabled();
    static float GetWidgetRightFraction();
    static bool GetWidgetBottomLeft(float& left, float& bottom);
    static bool BeginMap();
    static bool DrawTiles(int alpha);
    static void ProjectBlip(const void* worldPos, float& x, float& y);
    static void ReprojectFlatSprite(float& x, float& y);
    static void ProjectPlayerArrow(float& x, float& y);
};
