#include "../../../Main.h"
#include "CTiltedRadar.h"
#include "CTiltedCameraController.h"
#include "CTiltedRadarGeometry.h"

#include <mach-o/dyld.h>

bool CTiltedRadar::m_bEnabled = true;
bool CTiltedRadar::m_bBoundsValid = false;
float CTiltedRadar::m_fCenterX = 0.0f;
float CTiltedRadar::m_fCenterY = 0.0f;
float CTiltedRadar::m_fRadiusX = 0.0f;
float CTiltedRadar::m_fRadiusY = 0.0f;

static_assert(sizeof(RwIm2DVertex) == 28);
static_assert(offsetof(RwIm2DVertex, emissiveColor) == 16);
static_assert(sizeof(RwImVertexIndex) == 2);

static CTiltedCameraController cameraController;
static CTiltedCameraController::CameraState camState{};
static CTiltedVector3 cameraPos{};
static CTiltedVector3 cameraRot{};
static float radarOriginX = 0.0f;
static float radarOriginY = 0.0f;
static float radarRange = 0.0f;
static float cachedSin = 0.0f;
static float cachedCos = 1.0f;
static float spriteZ = 0.0f;
static float spriteRhw = 1.0f;
static std::intptr_t gtaSlide = 0;
static bool radarReady = false;
static bool drawingMap = false;
static bool drawingBlips = false;
static bool tiltedFrame = false;

static constexpr int SUBDIV_N = 14;
static constexpr int NUM_SUB_VERTS = (SUBDIV_N + 1) * (SUBDIV_N + 1);
static RwIm2DVertex subVertices[NUM_SUB_VERTS];
static bool vertValid[NUM_SUB_VERTS];

static constexpr int CLIP_SEGS = 32;
static float edgeX[CLIP_SEGS + 1];
static float edgeY[CLIP_SEGS + 1];
static float edgeA[CLIP_SEGS];
static float edgeB[CLIP_SEGS];
static float edgeC[CLIP_SEGS];
static float lastCenterX = -1.0f;
static float lastCenterY = -1.0f;
static float lastRadiusX = -1.0f;
static float lastRadiusY = -1.0f;

void CTiltedRadar::Initialise()
{
    const bool mapHooks = CMemory::HasMappedInstruction(SA_ADDR(0x1EFF00), "\x4F\x9C\x0A\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x1F0380), "\x34\x9B\x0A\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x2160B4), "\xEC\x03\x0A\x94");
    const bool blipHooks = CMemory::HasMappedInstruction(SA_ADDR(0x1F08E4), "\x56\x9A\x0A\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x1F0EEC), "\xD6\x98\x0A\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x1F2204), "\x14\x94\x0A\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x1F5CA0), "\x05\x85\x0A\x94");
    const bool widget = CMemory::HasMappedInstruction(SA_ADDR(0x1F033C), "\x28\x2D\x00\x90") && CMemory::HasMappedInstruction(SA_ADDR(0x1F0340), "\x08\x01\x35\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x1F0344), "\x08\x85\x42\xF9");
    const bool data = CMemory::HasMappedInstruction(SA_ADDR(0x1F56A0), "\x24\xD9\x4A\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x1F56B4), "\x25\x69\x45\xFD") && CMemory::HasMappedInstruction(SA_ADDR(0x1F56E0), "\x29\x11\x11\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x1F5648), "\x29\x01\x15\x91");
    const bool helpers = CMemory::HasMappedInstruction(SA_ADDR(0x190F1C), "\x88\x2D\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x1914A8), "\x88\x2D\x00\x90") && CMemory::HasMappedInstruction(SA_ADDR(0x302E1C), "\x20\x05\x46\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x302E2C), "\x21\x01\x46\xBD");

    radarReady = mapHooks && blipHooks && widget && data && helpers && RwRenderStateSet && RwRenderStateGet && RwIm2DRenderIndexedPrimitive;
    if (radarReady)
    {
        gtaSlide = _dyld_get_image_vmaddr_slide(0);
        CNecessary::Log(OBFUSCATE("CTiltedRadar Initialise"));
    }
}

void CTiltedRadar::SetEnabled(bool bEnabled)
{
    m_bEnabled = bEnabled;
}

bool CTiltedRadar::IsEnabled()
{
    return radarReady && m_bEnabled;
}

void CTiltedRadar::SetBounds(float cx, float cy, float rx, float ry)
{
    m_fCenterX = cx;
    m_fCenterY = cy;
    m_fRadiusX = rx;
    m_fRadiusY = ry;
    m_bBoundsValid = true;
}

bool CTiltedRadar::UpdateRadarWidgetBounds()
{
    m_bBoundsValid = false;
    const auto widget = *reinterpret_cast<const std::uintptr_t*>(SA_ADDR(0x795248) + gtaSlide);
    if (!widget) {
        return false;
    }

    const auto* rect = reinterpret_cast<const float*>(widget + 0x2C);
    const float rx = std::abs(rect[2] - rect[0]) * 0.5f;
    const float ry = std::abs(rect[3] - rect[1]) * 0.5f;
    const float cx = (rect[0] + rect[2]) * 0.5f;
    const float cy = (rect[1] + rect[3]) * 0.5f;
    if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(rx) || !std::isfinite(ry) || rx <= 10.0f || ry <= 10.0f) {
        return false;
    }

    CTiltedRadar::SetBounds(cx, cy, rx, ry);
    return true;
}

float CTiltedRadar::GetWidgetRightFraction()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x03BDD0), "\xE8\x31\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x03BDD4), "\x08\x01\x42\xF9") && CMemory::HasMappedInstruction(SA_ADDR(0x03BDE4), "\x00\x69\x69\xB8");
    if (!verified || !radarReady || !CTiltedRadar::UpdateRadarWidgetBounds()) {
        return 0.0f;
    }

    const int screenWidth = reinterpret_cast<int (*)()>(SA_ADDR(0x03BDD0) + gtaSlide)();
    const float radarRight = CTiltedRadar::m_fCenterX + CTiltedRadar::m_fRadiusX;
    return screenWidth > 0 && radarRight > 0.0f && radarRight < screenWidth ? radarRight / screenWidth : 0.0f;
}

bool CTiltedRadar::BeginMap()
{
    if (!CTiltedRadar::IsEnabled() || CGame::IsDrawingBigMap() || !CTiltedRadar::UpdateRadarWidgetBounds()) {
        return false;
    }

    // Stock Radar จะกำหนดค่าเหล่านี้ไว้ก่อนที่จะเรียกฟังก์ชัน DrawRadarMap
    const auto* origin = reinterpret_cast<const float*>(SA_ADDR(0x794AD0) + gtaSlide);
    const auto* trig = reinterpret_cast<const float*>(SA_ADDR(0x509444) + gtaSlide);
    radarOriginX = origin[0];
    radarOriginY = origin[1];
    radarRange = origin[2];
    cachedCos = trig[0];
    cachedSin = trig[1];
    if (!std::isfinite(radarOriginX) || !std::isfinite(radarOriginY) || !std::isfinite(radarRange) || !std::isfinite(cachedCos) || !std::isfinite(cachedSin) || std::abs(radarOriginX) > 20000.0f || std::abs(radarOriginY) > 20000.0f || radarRange <= 0.0f) {
        return false;
    }

    if (*reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x726364) + gtaSlide)) {
        return false;
    }

    const auto findPlayerVehicle = reinterpret_cast<const void* (*)(int, bool)>(SA_ADDR(0x190F1C) + gtaSlide);
    const auto findPlayerSpeed = reinterpret_cast<const float* (*)(int)>(SA_ADDR(0x1914A8) + gtaSlide);
    const bool inVehicle = findPlayerVehicle(-1, false) != nullptr;
    float speedKMH = 0.0f;
    if (inVehicle)
    {
        const auto* speed = findPlayerSpeed(-1);
        if (speed && std::isfinite(speed[0]) && std::isfinite(speed[1]))
        {
            speedKMH = std::hypot(speed[0], speed[1]) * 180.0f;
        }
    }

    const float yaw = -std::atan2(cachedSin, cachedCos);
    cameraController.Update(radarOriginX, radarOriginY, yaw, inVehicle, false, false, speedKMH);
    camState = cameraController.GetState();

    float offX = 0.0f;
    float offY = 0.0f;
    cameraController.GetCachedCalculations(offX, offY, cameraPos, cameraRot);
    return true;
}

bool CTiltedRadar::DrawTiles(int alpha)
{
    if (!m_bBoundsValid) {
        return false;
    }

    const RwRenderState states[] = {
        rwRENDERSTATETEXTURERASTER, rwRENDERSTATETEXTUREADDRESS, rwRENDERSTATETEXTUREFILTER,
        rwRENDERSTATEVERTEXALPHAENABLE, rwRENDERSTATESRCBLEND, rwRENDERSTATEDESTBLEND
    };

    void* saved[sizeof(states) / sizeof(states[0])]{};
    for (std::size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i)
    {
        if (!RwRenderStateGet(states[i], &saved[i])) {
            return false;
        }
    }

    spriteZ = *reinterpret_cast<const float*>(SA_ADDR(0x867604) + gtaSlide) + 1.0f;
    spriteRhw = *reinterpret_cast<const float*>(SA_ADDR(0x867600) + gtaSlide);
    const float rtSize = std::min(m_fRadiusX, m_fRadiusY) * 2.0f;
    const float aspect = CTiltedRadarMath::GetRadarProjectionAspect(false);
    const int centerCol = static_cast<int>(std::floor((radarOriginX + 3000.0f) / 500.0f));
    const int centerRow = static_cast<int>(std::floor((3000.0f - radarOriginY) / 500.0f));

    CTiltedRadar::DrawRadarBackgroundDisk(m_fCenterX, m_fCenterY, m_fRadiusX, m_fRadiusY, alpha);
    for (int row = centerRow - 2; row <= centerRow + 2; ++row)
    {
        for (int col = centerCol - 2; col <= centerCol + 2; ++col)
        {
            CTiltedRadar::Render3DMapTile(col, row, alpha, m_fCenterX, m_fCenterY, m_fRadiusX, m_fRadiusY, rtSize, aspect, radarOriginX, radarOriginY);
        }
    }

    for (std::size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i)
    {
        RwRenderStateSet(states[i], saved[i]);
    }

    return true;
}

bool CTiltedRadar::ProjectWorldToTiltedHud(float worldX, float worldY, float& outX, float& outY)
{
    CTiltedVector3 radarPos;
    CTiltedRadarGeometry::WorldToRadarPos(worldX, worldY, radarPos);
    const float rtSize = std::min(m_fRadiusX, m_fRadiusY) * 2.0f;
    const float aspect = CTiltedRadarMath::GetRadarProjectionAspect(false);

    if (!CTiltedRadarGeometry::WorldToCircleScreen(radarPos, cameraPos, cameraRot, camState.fov, 0.3f, 10000.0f, rtSize, rtSize, m_fRadiusX * 2.0f, m_fRadiusY * 2.0f, m_fCenterX, m_fCenterY, outX, outY, aspect))
    {
        const CTiltedVector3 playerRadarPos(camState.posX, camState.posY, 0.0f);
        float angle = 0.0f;

        if (!CTiltedRadarMath::DirectionToOrbitAngle(playerRadarPos, radarPos, camState.yaw, angle)) {
            return false;
        }

        CTiltedRadarGeometry::PointOnOrbitEdge(m_fCenterX, m_fCenterY, m_fRadiusX, m_fRadiusY, std::cos(angle), std::sin(angle), false, outX, outY);
    }
    else if (!CTiltedRadarGeometry::IsInsideOrbit(outX, outY, m_fCenterX, m_fCenterY, m_fRadiusX, m_fRadiusY, false))
    {
        CTiltedRadarGeometry::ClampToOrbit(outX, outY, m_fCenterX, m_fCenterY, m_fRadiusX, m_fRadiusY, outX, outY, false);
    }

    return std::isfinite(outX) && std::isfinite(outY);
}

void CTiltedRadar::ReprojectFlatSprite(float& x, float& y)
{
    const float rx = (x - m_fCenterX) / m_fRadiusX;
    const float ry = (m_fCenterY - y) / m_fRadiusY;
    const float worldX = radarOriginX + (cachedCos * rx - cachedSin * ry) * radarRange;
    const float worldY = radarOriginY + (cachedSin * rx + cachedCos * ry) * radarRange;
    float projectedX = 0.0f;
    float projectedY = 0.0f;
    if (CTiltedRadar::ProjectWorldToTiltedHud(worldX, worldY, projectedX, projectedY))
    {
        x = projectedX;
        y = projectedY;
    }
}

void CTiltedRadar::ProjectBlip(const void* worldPos, float& x, float& y)
{
    if (worldPos)
    {
        CTiltedVector2 position;
        std::memcpy(&position, worldPos, sizeof(position));
        float projectedX = 0.0f;
        float projectedY = 0.0f;
        if (CTiltedRadar::ProjectWorldToTiltedHud(position.x, position.y, projectedX, projectedY))
        {
            x = projectedX;
            y = projectedY;
        }
    }
    else
    {
        CTiltedRadar::ReprojectFlatSprite(x, y);
    }
}

void CTiltedRadar::ProjectPlayerArrow(float& x, float& y)
{
    if (!CTiltedRadar::ProjectWorldToTiltedHud(radarOriginX, radarOriginY, x, y))
    {
        x = m_fCenterX;
        y = m_fCenterY + m_fRadiusY * 0.32f;
    }
}

extern "C" __attribute__((visibility("default"))) void CRadar_DrawRadarMap_hook(void (*original)())
{
    tiltedFrame = false;
    drawingMap = CTiltedRadar::BeginMap();
    original();
    drawingMap = false;
}

extern "C" __attribute__((visibility("default"))) void CRadar_DrawRadarSection_hook(int x, int y, int alpha, void (*original)(int, int, int))
{
    if (drawingMap)
    {
        if (tiltedFrame) {
            return;
        }

        tiltedFrame = CTiltedRadar::DrawTiles(alpha);
        if (tiltedFrame) {
            return;
        }

        drawingMap = false;
    }

    original(x, y, alpha);
}

extern "C" __attribute__((visibility("default"))) void CRadar_DrawBlips_hook(float scale, void (*original)(float))
{
    drawingBlips = tiltedFrame && !CGame::IsDrawingBigMap();
    original(scale);
    drawingBlips = false;
    tiltedFrame = false;
}

extern "C" __attribute__((visibility("default"))) void CRadar_DrawRadarSprite_hook(int id, float x, float y, int alpha, void (*original)(int, float, float, int), const void* worldPos)
{
    if (drawingBlips && id != 4)
    {
        CTiltedRadar::ProjectBlip(worldPos, x, y);
    }

    original(id, x, y, alpha);
}

extern "C" __attribute__((visibility("default"))) void CRadar_DrawRotatingRadarSprite_hook(const void* sprite, float x, float y, float angle, float scale, const void* color, void (*original)(const void*, float, float, float, float, const void*), const void* worldPos)
{
    if (drawingBlips)
    {
        const auto* playerArrow = reinterpret_cast<const void*>(SA_ADDR(0x794B10) + gtaSlide);
        if (sprite == playerArrow)
        {
            CTiltedRadar::ProjectPlayerArrow(x, y);
        }
        else
        {
            CTiltedRadar::ProjectBlip(worldPos, x, y);
        }
    }

    original(sprite, x, y, angle, scale, color);
}

extern "C" __attribute__((visibility("default"))) void CRadar_ShowRadarTraceWithHeight_hook(float x, float y, int size, int r, int g, int b, int a, int height, void (*original)(float, float, int, int, int, int, int, int), const void* worldPos)
{
    if (drawingBlips)
    {
        CTiltedRadar::ProjectBlip(worldPos, x, y);
    }

    original(x, y, size, r, g, b, a, height);
}

extern "C" __attribute__((visibility("default"))) void CRadar_SetAreaVertices_hook(int count, const CTiltedVector2* xy, const CTiltedVector2* uv, const void* color, void (*original)(int, const CTiltedVector2*, const CTiltedVector2*, const void*))
{
    if (drawingMap && tiltedFrame && count >= 3 && count <= 8)
    {
        CTiltedVector2 projected[8];
        for (int i = 0; i < count; ++i)
        {
            projected[i] = xy[i];
            CTiltedRadar::ReprojectFlatSprite(projected[i].x, projected[i].y);
        }

        original(count, projected, uv, color);
        return;
    }

    original(count, xy, uv, color);
}

void CTiltedRadar::UpdateRadarEdgeTable(float centerX, float centerY, float radiusX, float radiusY)
{
    if (centerX != lastCenterX || centerY != lastCenterY || radiusX != lastRadiusX || radiusY != lastRadiusY)
    {
        lastCenterX = centerX;
        lastCenterY = centerY;
        lastRadiusX = radiusX;
        lastRadiusY = radiusY;
        const float clipRX = radiusX * 0.97f;
        const float clipRY = radiusY * 0.97f;
        for (int i = 0; i <= CLIP_SEGS; ++i)
        {
            float a = (static_cast<float>(i) / static_cast<float>(CLIP_SEGS)) * 6.28318530718f;
            edgeX[i] = centerX + cosf(a) * clipRX;
            edgeY[i] = centerY + sinf(a) * clipRY;
        }

        for (int e = 0; e < CLIP_SEGS; ++e)
        {
            float ex0 = edgeX[e];
            float ey0 = edgeY[e];
            float ex1 = edgeX[e + 1];
            float ey1 = edgeY[e + 1];
            float edx = ex1 - ex0;
            float edy = ey1 - ey0;

            float centerSide = edx * (centerY - ey0) - edy * (centerX - ex0);
            float sgn = (centerSide >= 0.0f) ? 1.0f : -1.0f;

            edgeA[e] = -sgn * edy;
            edgeB[e] = sgn * edx;
            edgeC[e] = sgn * (edy * ex0 - edx * ey0);
        }
    }
}

void CTiltedRadar::DrawRadarBackgroundDisk(float centerX, float centerY, float radiusX, float radiusY, int alpha)
{
    CTiltedRadar::UpdateRadarEdgeTable(centerX, centerY, radiusX, radiusY);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(1));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void*>(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void*>(rwBLENDINVSRCALPHA));

    uint32_t waterColor = (static_cast<uint32_t>(alpha & 0xFF) << 24) | 0x00AA896F;

    static RwIm2DVertex bgVerts[CLIP_SEGS + 1];
    static RwImVertexIndex bgIndices[CLIP_SEGS * 3];
    static bool bgIndicesInit = false;

    if (!bgIndicesInit)
    {
        for (int i = 0; i < CLIP_SEGS; ++i)
        {
            bgIndices[i * 3 + 0] = 0;
            bgIndices[i * 3 + 1] = static_cast<RwImVertexIndex>(i + 1);
            bgIndices[i * 3 + 2] = static_cast<RwImVertexIndex>((i + 1 == CLIP_SEGS) ? 1 : (i + 2));
        }

        bgIndicesInit = true;
    }

    bgVerts[0].x = centerX;
    bgVerts[0].y = centerY;
    bgVerts[0].z = spriteZ;
    bgVerts[0].rhw = spriteRhw;
    bgVerts[0].u = 0.0f;
    bgVerts[0].v = 0.0f;
    bgVerts[0].emissiveColor = waterColor;

    for (int i = 0; i < CLIP_SEGS; ++i)
    {
        bgVerts[i + 1].x = edgeX[i];
        bgVerts[i + 1].y = edgeY[i];
        bgVerts[i + 1].z = spriteZ;
        bgVerts[i + 1].rhw = spriteRhw;
        bgVerts[i + 1].u = 0.0f;
        bgVerts[i + 1].v = 0.0f;
        bgVerts[i + 1].emissiveColor = waterColor;
    }

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, bgVerts, CLIP_SEGS + 1, bgIndices, CLIP_SEGS * 3);
}

void CTiltedRadar::Render3DMapTile(int sectionX, int sectionY, int alpha, float centerX, float centerY, float radiusX, float radiusY, float rtSize, float aspect, float playerWorldX, float playerWorldY)
{
    if (sectionX < 0 || sectionX > 11 || sectionY < 0 || sectionY > 11) {
        return;
    }

    float tileLeft = -3000.0f + static_cast<float>(sectionX) * 500.0f;
    float tileRight = tileLeft + 500.0f;
    float tileBottom = -3000.0f + static_cast<float>(11 - sectionY) * 500.0f;
    float tileTop = tileBottom + 500.0f;

    float pNearX = (playerWorldX < tileLeft) ? tileLeft : ((playerWorldX > tileRight) ? tileRight : playerWorldX);
    float pNearY = (playerWorldY < tileBottom) ? tileBottom : ((playerWorldY > tileTop) ? tileTop : playerWorldY);
    float pDistSq = (pNearX - playerWorldX) * (pNearX - playerWorldX) + (pNearY - playerWorldY) * (pNearY - playerWorldY);

    float camWorldX = cameraPos.x - 3000.0f;
    float camWorldY = cameraPos.y + 3000.0f;
    float cNearX = (camWorldX < tileLeft) ? tileLeft : ((camWorldX > tileRight) ? tileRight : camWorldX);
    float cNearY = (camWorldY < tileBottom) ? tileBottom : ((camWorldY > tileTop) ? tileTop : camWorldY);
    float cDistSq = (cNearX - camWorldX) * (cNearX - camWorldX) + (cNearY - camWorldY) * (cNearY - camWorldY);

    constexpr float MAX_VISIBLE_DIST_SQ = 850.0f * 850.0f;
    if (pDistSq > MAX_VISIBLE_DIST_SQ && cDistSq > MAX_VISIBLE_DIST_SQ) {
        return;
    }

    const auto* textures = reinterpret_cast<RwTexture* const*>(SA_ADDR(0x791540) + gtaSlide);
    const auto* texture = textures[sectionY * 12 + sectionX];
    if (!texture) {
        return;
    }

    auto* pRaster = texture->raster;
    if (!pRaster) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pRaster);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, reinterpret_cast<void*>(rwTEXTUREADDRESSCLAMP));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, reinterpret_cast<void*>(rwFILTERLINEAR));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(1));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void*>(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void*>(rwBLENDINVSRCALPHA));

    uint32_t vertColor = (static_cast<uint32_t>(alpha & 0xFF) << 24) | 0x00FFFFFF;

    int vertIndex = 0;
    for (int j = 0; j <= SUBDIV_N; ++j)
    {
        float vRatio = static_cast<float>(j) / static_cast<float>(SUBDIV_N);
        float worldY = tileTop - vRatio * 500.0f;

        for (int i = 0; i <= SUBDIV_N; ++i)
        {
            float uRatio = static_cast<float>(i) / static_cast<float>(SUBDIV_N);
            float worldX = tileLeft + uRatio * 500.0f;

            CTiltedVector3 radarPos;
            CTiltedRadarGeometry::WorldToRadarPos(worldX, worldY, radarPos);

            float screenX = 0.0f;
            float screenY = 0.0f;
            bool bVisible = CTiltedRadarGeometry::WorldToCircleScreen(radarPos, cameraPos, cameraRot, camState.fov, 0.3f, 10000.0f, rtSize, rtSize, radiusX * 2.0f, radiusY * 2.0f, centerX, centerY, screenX, screenY, aspect);

            RwIm2DVertex& v = subVertices[vertIndex];
            if (bVisible)
            {
                float dx = screenX - centerX;
                float dy = screenY - centerY;
                float distSq = dx * dx + dy * dy;

                if (distSq > 2500.0f * 2500.0f)
                {
                    vertValid[vertIndex] = false;
                }
                else
                {
                    vertValid[vertIndex] = true;
                    v.x = screenX;
                    v.y = screenY;
                }
            }
            else
            {
                vertValid[vertIndex] = false;
            }

            v.z = spriteZ;
            v.rhw = spriteRhw;
            v.u = uRatio;
            v.v = vRatio;
            v.emissiveColor = vertColor;
            vertIndex++;
        }
    }

    static constexpr int MAX_CLIP_POLY = 3 + CLIP_SEGS + 4;
    static constexpr int MAX_OUT_VERTS = 384;

    struct ClipVert
    {
        float x, y, u, v;
    };

    static RwIm2DVertex s_outVerts[MAX_OUT_VERTS];
    static RwImVertexIndex s_outIdx[MAX_OUT_VERTS];

    CTiltedRadar::UpdateRadarEdgeTable(centerX, centerY, radiusX, radiusY);

    const float innerK = cosf(3.14159265359f / static_cast<float>(CLIP_SEGS));
    int outCount = 0;

    auto flush = [&]()
    {
        if (outCount <= 0)
        {
            outCount = 0;
            return;
        }

        for (int i = 0; i < outCount; ++i)
        {
            s_outIdx[i] = static_cast<RwImVertexIndex>(i);
        }

        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, s_outVerts, outCount, s_outIdx, outCount);
        outCount = 0;
    };

    auto emitVert = [&](const ClipVert& c)
    {
        RwIm2DVertex& o = s_outVerts[outCount++];
        o.x = c.x;
        o.y = c.y;
        o.z = spriteZ;
        o.rhw = spriteRhw;
        o.u = c.u;
        o.v = c.v;
        o.emissiveColor = vertColor;
    };

    const float clipRX = radiusX * 0.97f;
    const float clipRY = radiusY * 0.97f;
    const float clipMinX = centerX - clipRX;
    const float clipMaxX = centerX + clipRX;
    const float clipMinY = centerY - clipRY;
    const float clipMaxY = centerY + clipRY;
    const float invClipRX = 1.0f / clipRX;
    const float invClipRY = 1.0f / clipRY;
    const float innerKSq = innerK * innerK;

    auto isInsideEllipse = [&](float x, float y, float kSq) -> bool
    {
        float nx = (x - centerX) * invClipRX;
        float ny = (y - centerY) * invClipRY;
        return (nx * nx + ny * ny) <= kSq;
    };

    auto clipAndEmitTri = [&](const RwIm2DVertex& a, const RwIm2DVertex& b, const RwIm2DVertex& c)
    {
        float triMinX = fminf(a.x, fminf(b.x, c.x));
        float triMaxX = fmaxf(a.x, fmaxf(b.x, c.x));
        float triMinY = fminf(a.y, fminf(b.y, c.y));
        float triMaxY = fmaxf(a.y, fmaxf(b.y, c.y));
        if (triMaxX < clipMinX || triMinX > clipMaxX || triMaxY < clipMinY || triMinY > clipMaxY) {
            return;
        }

        ClipVert polyA[MAX_CLIP_POLY];
        ClipVert polyB[MAX_CLIP_POLY];
        polyA[0] = { a.x, a.y, a.u, a.v };
        polyA[1] = { b.x, b.y, b.u, b.v };
        polyA[2] = { c.x, c.y, c.u, c.v };
        int n = 3;

        if (isInsideEllipse(a.x, a.y, innerKSq) && isInsideEllipse(b.x, b.y, innerKSq) && isInsideEllipse(c.x, c.y, innerKSq))
        {
            if (outCount + 3 > MAX_OUT_VERTS)
            {
                flush();
            }

            emitVert(polyA[0]);
            emitVert(polyA[1]);
            emitVert(polyA[2]);
            return;
        }

        ClipVert* src = polyA;
        ClipVert* dst = polyB;

        for (int e = 0; e < CLIP_SEGS && n > 0; ++e)
        {
            const float eA = edgeA[e];
            const float eB = edgeB[e];
            const float eC = edgeC[e];

            int m = 0;
            for (int i = 0; i < n; ++i)
            {
                const ClipVert& P = src[i];
                const ClipVert& Q = src[(i + 1) % n];
                float dP = eA * P.x + eB * P.y + eC;
                float dQ = eA * Q.x + eB * Q.y + eC;
                bool inP = dP >= 0.0f;
                bool inQ = dQ >= 0.0f;

                if (inP && m < MAX_CLIP_POLY)
                {
                    dst[m++] = P;
                }

                if (inP != inQ && m < MAX_CLIP_POLY)
                {
                    float t = dP / (dP - dQ);
                    ClipVert I;
                    I.x = P.x + (Q.x - P.x) * t;
                    I.y = P.y + (Q.y - P.y) * t;
                    I.u = P.u + (Q.u - P.u) * t;
                    I.v = P.v + (Q.v - P.v) * t;
                    dst[m++] = I;
                }
            }

            n = m;
            ClipVert* tmp = src;
            src = dst;
            dst = tmp;
        }

        if (n < 3) {
            return;
        }

        int needed = (n - 2) * 3;
        if (outCount + needed > MAX_OUT_VERTS)
        {
            flush();
        }

        for (int i = 1; i + 1 < n; ++i)
        {
            emitVert(src[0]);
            emitVert(src[i]);
            emitVert(src[i + 1]);
        }
    };

    for (int y = 0; y < SUBDIV_N; ++y)
    {
        for (int x = 0; x < SUBDIV_N; ++x)
        {
            int vTL = y * (SUBDIV_N + 1) + x;
            int vTR = vTL + 1;
            int vBL = (y + 1) * (SUBDIV_N + 1) + x;
            int vBR = vBL + 1;

            if (!vertValid[vTL] || !vertValid[vTR] || !vertValid[vBL] || !vertValid[vBR]) {
                continue;
            }

            const RwIm2DVertex& pTL = subVertices[vTL];
            const RwIm2DVertex& pTR = subVertices[vTR];
            const RwIm2DVertex& pBL = subVertices[vBL];
            const RwIm2DVertex& pBR = subVertices[vBR];

            float area = fabsf((pTR.x - pTL.x) * (pBL.y - pTL.y) - (pTR.y - pTL.y) * (pBL.x - pTL.x));
            if (area < 0.5f) {
                continue;
            }

            if (((x + y) & 1) == 0)
            {
                clipAndEmitTri(pTL, pTR, pBR);
                clipAndEmitTri(pTL, pBR, pBL);
            }
            else
            {
                clipAndEmitTri(pTL, pTR, pBL);
                clipAndEmitTri(pTR, pBR, pBL);
            }
        }
    }

    flush();
}
