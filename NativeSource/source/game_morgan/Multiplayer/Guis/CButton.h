#pragma once

#include <mutex>
#include <vector>
#include "../../Enums/eKeysBarKey.h"

class CButton
{
public:
    CButton();
	~CButton();

	void Render();
    void Enable(bool status);

    static bool QueueKeyInput(int keyId);

    static void SetKeyF4(bool enable);
    static void SetKeyUPDOWN(bool enable);
    static void SetKeyLEFTRIGHT(bool enable);

public:
    bool m_bKeysStatus;
    bool m_bRenderStatus;
    bool m_bKeyAfterDeath;

private:
    bool m_bStateSent = false;
    static void SendKeyInput(int keyId);
    static std::mutex m_KeyInputMutex;
    static std::vector<int> m_KeyInputQueue;

    static bool m_bToggleKeyF4;
    static bool m_bToggleKeyUPDOWN;
    static bool m_bToggleKeyLEFTRIGHT;
};