#pragma once

#include "../../../Audio/CBass.h"
#include "../../../Header/Common.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class CAudioStream
{
public:
    CAudioStream();
    ~CAudioStream();

    void Play(const char* streamUrl, float posX, float posY, float posZ, float streamRadius, bool isPositionBased);
    void Stop(bool shouldWaitForThread = true);
    void Process();
    void ControlGameRadio();
    void SetVolume(std::uint32_t volume);
    bool GetStreamTags() const;
    void SetAudioMsg(bool toggle);

    char m_streamLabel[256]{};

private:
    void Initialize();
    void StreamThread();
    void CancelConnection();
    static void AudioStreamMetaSync(HSYNC, std::uint32_t streamChannel, std::uint32_t, void* userData);
    void UpdateStreamTags();

    std::uint32_t m_lastAudioStreamUpdateTick = 0;
    std::uint32_t m_lastGlobalVolume = 0;
    HSTREAM bassStreamHandle = 0;
    VECTOR m_audioStreamPosition{};
    float m_audioStreamRadius = 0.0f;
    bool m_isAudioStreamPositionBased = false;
    bool m_bInitialized = false;
    bool m_bAudioMsg = false;

    std::mutex m_tagsMutex;
    HSTREAM m_tagsChannel = 0;
    char m_streamTitle[256]{};

    std::thread m_audioStreamThread;
    std::mutex m_mutex;
    std::condition_variable m_requestChanged;
    std::string m_audioStreamUrl;
    std::uint64_t m_request = 0;
    bool m_bIsPlaying = false;
    bool m_workerBusy = false;
    bool m_connecting = false;
    bool m_cancelPending = false;
    bool m_stopping = false;
};