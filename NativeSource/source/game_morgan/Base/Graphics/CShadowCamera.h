#pragma once

#include "../../RenderWare/rwcore.h"

class CShadowCamera
{
public:
	RwCamera* m_pShadowCamera;
	RwTexture* m_pShadowTexture;
};
static_assert(sizeof(CShadowCamera) == 0x10);