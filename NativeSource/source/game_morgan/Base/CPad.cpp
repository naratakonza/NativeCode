#include "../../../Main.h"
#include "CPad.h"
#include "../CCameraController.h"
#include "../CGyro.h"
#include "../CSensitivity.h"
#include "../CTimer.h"
#include "../Multiplayer/CMultiPlayer.h"

#include <mach-o/dyld.h>

void CPad::DisplayWidgets(bool bDisp)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15B6F8), "\xC8\x2E\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15B6FC), "\x08\xF1\x28\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x43AE90), "\x08\x20\x42\x79");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    *reinterpret_cast<std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + slide) = static_cast<std::uint16_t>(!bDisp); // Pads[0].DisablePlayerControls
}

bool CPad::IsActiveWidgets()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15B6F8), "\xC8\x2E\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15B6FC), "\x08\xF1\x28\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x43AE90), "\x08\x20\x42\x79");
    return verified && *reinterpret_cast<const std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + _dyld_get_image_vmaddr_slide(0)) != 0;
}

extern "C" __attribute__((visibility("default"))) float CPad_AimWeaponLeftRight_hook(uintptr_t thiz, CPedGta* pPed, bool *pbTouchInputX, float (*CPad_AimWeaponLeftRight)(uintptr_t, CPedGta*, bool*))
{
	if (!pthread_main_np() || !CMultiPlayer::GetActiveNetwork() || CTimer::IsUserPause()) {
		return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
	}

	if (CGyro::IsGyro())
	{
		float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
		float aimSens = CGyro::GetAimSensitivity() / 100.0f;
		float onfootSens = CGyro::GetOnfootSensitivity() / 100.0f;
		float inCarSens = CGyro::GetIncarSensitivity() / 100.0f;

		float gyroY = CGyro::GetGyroY();
		if (std::fabs(gyroY) < 0.015f) 
		{
			gyroY = 0.0f; // โซนไร้การสั่นสะเทือนเพื่อกรองการสั่นสะเทือนเล็กน้อย
		}

		const double elapsedTime = CGyro::GetElapsedTime();

		float gyroOffset;
		if (CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * aimSens;
				return (tmp + gyroOffset) * aimSens;
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return tmp + gyroOffset;
				}
				else
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return (tmp * (CSensitivity::GetIncarTouch() / 100.0f)) + gyroOffset;
				}
			}
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * aimSens;
				return (tmp + gyroOffset) * aimSens;
			}

			return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_ONFOOT_AND_INCAR_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				float aimInput = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
				return aimInput * (CSensitivity::GetAimTouch() / 100.0f);
			}
			
			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return tmp + gyroOffset;
				}
				else
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return (tmp * (CSensitivity::GetIncarTouch() / 100.0f)) + gyroOffset;
				}
			}
		}
	}
	else
	{
		if (CCameraController::IsCameraAllAiming(false, false))
		{
			float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
			return tmp * (CSensitivity::GetAimTouch() / 100.0f);
		}

		if (CGame::GetPlayer() && CGame::GetPlayer()->IsInVehicle())
		{
			float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
			return tmp * (CSensitivity::GetIncarTouch() / 100.0f);
		}

		return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
	}

	return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
}

extern "C" __attribute__((visibility("default"))) int16_t CPad_AimWeaponUpDown_hook(uintptr_t thiz, CPedGta* pPed, bool *pbTouchInputX, int16_t (*CPad_AimWeaponUpDown)(uintptr_t, CPedGta*, bool*))
{
	if (!pthread_main_np() || !CMultiPlayer::GetActiveNetwork() || CTimer::IsUserPause()) {
		return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
	}

	const auto clampInput = [](float value)
	{
		return static_cast<int16_t>(std::clamp(value, static_cast<float>(INT16_MIN), static_cast<float>(INT16_MAX)));
	};

	if (CGyro::IsGyro())
	{
		int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
		float aimSens = CGyro::GetAimSensitivity() / 100.0f;
		float onfootSens = CGyro::GetOnfootSensitivity() / 100.0f;
		float inCarSens = CGyro::GetIncarSensitivity() / 100.0f;

		float gyroX = CGyro::GetGyroX();
		if (std::fabs(gyroX) < 0.015f)
		{
			gyroX = 0.0f; // โซนไร้การสั่นสะเทือนเพื่อกรองการสั่นสะเทือนเล็กน้อย
		}

		const double elapsedTime = CGyro::GetElapsedTime();

		float gyroOffset;
		if (CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * aimSens;
				float scaled = (static_cast<float>(tmp) + gyroOffset) * aimSens;
				return clampInput(scaled);
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return clampInput(static_cast<float>(tmp) + gyroOffset);
				}
				else
				{
					gyroOffset = gyroX * 3000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return clampInput(static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f) + gyroOffset);
				}
			}
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * aimSens;
				float scaled = (static_cast<float>(tmp) + gyroOffset) * aimSens;
				return clampInput(scaled);
			}

			return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_ONFOOT_AND_INCAR_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				int16_t aimInput = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
				float scaled = static_cast<float>(aimInput) * (CSensitivity::GetAimTouch() / 100.0f);

				return clampInput(scaled);
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return clampInput(static_cast<float>(tmp) + gyroOffset);
				}
				else
				{
					gyroOffset = gyroX * 3000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return clampInput(static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f) + gyroOffset);
				}
			}
		}
	}
	else
	{
		if (CCameraController::IsCameraAllAiming(false, false))
		{
			int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
			float scaled = static_cast<float>(tmp) * (CSensitivity::GetAimTouch() / 100.0f);

			return clampInput(scaled);
		}

		if (CGame::GetPlayer() && CGame::GetPlayer()->IsInVehicle())
		{
			int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
			float scaled = static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f);

			return clampInput(scaled);
		}

		return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
	}

	return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
}