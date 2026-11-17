#include "../../Main.h"
#include "CGyro.h"
#include "Enums/eMaxConfig.h"

#include <mach-o/dyld.h>

bool CGyro::m_bEnable = false;
float CGyro::m_fGyroX = 0.0f;
float CGyro::m_fGyroY = 0.0f;
float CGyro::m_fGyroZ = 0.0f;
float CGyro::ms_fAimSens = 170.0f;
float CGyro::ms_fOnfootSens = 170.0f;
float CGyro::ms_fIncarSens = 170.0f;
eGyroMode CGyro::m_ntype = eGyroMode::GYRO_ALL_MODE;

void CGyro::SetGyroMode(eGyroMode mode)
{
    if (mode > eGyroMode::GYRO_ALL_MODE)
    {
        mode = eGyroMode::GYRO_ALL_MODE;
    }

    CGyro::m_ntype = mode;
}

eGyroMode CGyro::GetGyroMode()
{
    return CGyro::m_ntype;
}

void CGyro::SetAimSensitivity(float value)
{
    if (std::isfinite(value))
    {
        CGyro::ms_fAimSens = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    }
}

float CGyro::GetAimSensitivity()
{
	return CGyro::ms_fAimSens;
}

void CGyro::SetOnfootSensitivity(float value)
{
    if (std::isfinite(value))
    {
        CGyro::ms_fOnfootSens = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    }
}

float CGyro::GetOnfootSensitivity()
{
	return CGyro::ms_fOnfootSens;
}

void CGyro::SetIncarSensitivity(float value)
{
    if (std::isfinite(value))
    {
        CGyro::ms_fIncarSens = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    }
}

float CGyro::GetIncarSensitivity()
{
	return CGyro::ms_fIncarSens;
}

void CGyro::SetGyroMoveSpeed(float x, float y, float z)
{
    CGyro::m_fGyroX = x;
    CGyro::m_fGyroY = y;
    CGyro::m_fGyroZ = z;
}

float CGyro::GetGyroX()
{ 
    return CGyro::m_fGyroX; 
}

float CGyro::GetGyroY()
{ 
    return CGyro::m_fGyroY; 
}

float CGyro::GetGyroZ()
{ 
    return CGyro::m_fGyroZ; 
}

double CGyro::GetElapsedTime()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x17FA8C), "\x00\x90\x26\x1E\x20\x18\x20\x1E\x08\x2E\x00\xD0\x00\x3D\x0A\xBD", 16, "__text");
    if (!verified) {
        return 0.016;
    }

    const auto* timeStep = reinterpret_cast<const float*>(SA_ADDR(0x741A3C) + _dyld_get_image_vmaddr_slide(0));
    return std::isfinite(*timeStep) && *timeStep > 0.0f ? *timeStep * 0.02 : 0.016;
}

void CGyro::Enable(bool enable)
{
    CGyro::m_bEnable = enable;
    if (!enable)
    {
        CGyro::SetGyroMoveSpeed(0.0f, 0.0f, 0.0f);
    }
}

bool CGyro::IsGyro()
{
    return CGyro::m_bEnable;
}