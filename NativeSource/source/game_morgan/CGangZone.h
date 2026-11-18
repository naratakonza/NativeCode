#pragma once

class CGangZone
{
private:
	float 		m_fStartX, m_fStartY;
	float 		m_fEndX, m_fEndY;
	uint32_t 	m_dwColor;
	uint32_t  	m_dwAltColor;

public:
	CGangZone(float fStartX, float fStartY, float fEndX, float fEndY, uint32_t dwColor, uint32_t dwAltColor);
	~CGangZone();

	void Draw(bool inMenu, uint8_t alt);
	void Flash(uint32_t dwColor);
	void StopFlash();

	float GetStartX() const { return m_fStartX; }
	float GetStartY() const { return m_fStartY; }
	float GetEndX() const { return m_fEndX; }
	float GetEndY() const { return m_fEndY; }
	float GetArea() const { 
		return (m_fEndX > m_fStartX ? (m_fEndX - m_fStartX) : (m_fStartX - m_fEndX)) * (m_fEndY > m_fStartY ? (m_fEndY - m_fStartY) : (m_fStartY - m_fEndY)); 
	}
};