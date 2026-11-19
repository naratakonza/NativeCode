#pragma once

#include "../../Header/Common.h"

struct RwTexture;
struct RwCamera;
struct RpLight;
struct RwFrame;
struct RwRaster;

class CModelSnapshot
{
public:
    CModelSnapshot();
    ~CModelSnapshot();

    RwTexture* CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);
    // RwTexture* CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom, int dwColor1, int dwColor2);
    // RwTexture* CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);

private:
    void SetUpScene();

    RwCamera* m_camera;
    RpLight* m_light;
    RwFrame* m_frame;
    RwRaster* m_zBuffer;
};