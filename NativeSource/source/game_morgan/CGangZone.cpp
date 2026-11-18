#include "../../Main.h"
#include "CGangZone.h"
#include "CRadar.h"
#include "CRect.h"

CGangZone::CGangZone(float fStartX, float fStartY, float fEndX, float fEndY, uint32_t dwColor, uint32_t dwAltColor)
{
    CGangZone::m_fStartX = fStartX;
    CGangZone::m_fStartY = fStartY;
    CGangZone::m_fEndX = fEndX;
    CGangZone::m_fEndY = fEndY;
    CGangZone::m_dwColor = dwColor;
    CGangZone::m_dwAltColor = dwAltColor;
}

CGangZone::~CGangZone()
{
    CGangZone::m_fStartX = 0.0f;
	CGangZone::m_fStartY = 0.0f;
	CGangZone::m_fEndX = 0.0f;
	CGangZone::m_fEndY = 0.0f;
	CGangZone::m_dwColor = 0x0;
	CGangZone::m_dwAltColor = 0x0;
}

void CGangZone::Draw(bool inMenu, uint8_t alt) 
{
	uint32_t dwCurColor = alt ? CGangZone::m_dwAltColor : CGangZone::m_dwColor;

	CRect rect;
	rect.X1 = CGangZone::m_fStartX;
	rect.Y1 = CGangZone::m_fStartY;
	rect.X2 = CGangZone::m_fEndX;
	rect.Y2 = CGangZone::m_fEndY;

    CRadar::DrawAreaOnRadar(rect, dwCurColor, inMenu);
}

void CGangZone::Flash(uint32_t dwColor)
{
	CGangZone::m_dwAltColor = dwColor;
}

void CGangZone::StopFlash()
{
	CGangZone::m_dwAltColor = CGangZone::m_dwColor;
}