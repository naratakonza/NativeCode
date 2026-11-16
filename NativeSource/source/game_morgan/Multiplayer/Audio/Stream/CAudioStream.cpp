#include "../../../../../Main.h"
#include "CAudioStream.h"
#include "../../../Audio/CAEAudioHardware.h"
#include "../../../Audio/CAudioEngine.h"
#include "../../Guis/CGui.h"

#include <chrono>
#include <utility>

static constexpr std::uint32_t BASS_CONFIG_NET_TIMEOUT = 11;
static constexpr std::uint32_t BASS_CONFIG_NET_AGENT = 16;
static constexpr std::uint32_t BASS_CONFIG_NET_PROXY = 17;
static constexpr std::uint32_t BASS_CONFIG_NET_PLAYLIST = 21;
static constexpr std::uint32_t BASS_CONFIG_NET_READTIMEOUT = 37;
static constexpr std::uint32_t BASS_CONFIG_IOS_SESSION = 34;
static constexpr std::uint32_t BASS_IOS_SESSION_DISABLE = 0x10;
static constexpr std::uint32_t BASS_STREAM_BLOCK = 0x100000;
static constexpr std::uint32_t BASS_STREAM_STATUS = 0x800000;
static constexpr std::uint32_t BASS_SYNC_META = 4;
static constexpr std::uint32_t BASS_SYNC_OGG_CHANGE = 12;
static constexpr std::uint32_t BASS_ATTRIB_VOL = 2;
static constexpr std::uint32_t BASS_ACTIVE_STOPPED = 0;
static constexpr std::uint32_t BASS_TAG_OGG = 2;
static constexpr std::uint32_t BASS_TAG_HTTP = 3;
static constexpr std::uint32_t BASS_TAG_ICY = 4;
static constexpr std::uint32_t BASS_TAG_META = 5;

CAudioStream::CAudioStream()
{
    CNecessary::Log(OBFUSCATE("CAudioStream Initialise"));
    CAudioStream::Initialize();
}

CAudioStream::~CAudioStream()
{
    CNecessary::Log(OBFUSCATE("CAudioStream Uninitialise"));
    CAudioStream::Stop(true);

    if (CAudioStream::m_audioStreamThread.joinable())
    {
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }

        m_requestChanged.notify_one();
        CAudioStream::m_audioStreamThread.join();
    }

    if (CAudioStream::m_bInitialized)
    {
        BASS_Free();
    }
}

void CAudioStream::Initialize()
{
    if (!BASS_Init) {
        return;
    }

    BASS_SetConfig(BASS_CONFIG_IOS_SESSION, BASS_IOS_SESSION_DISABLE);
    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr))
    {
        CNecessary::Log(OBFUSCATE("CAudioStream: BASS_Init failed: %d"), BASS_ErrorGetCode());
        return;
    }

    BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, "SA-MP/0.3");
    BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, nullptr);
    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
    BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 10000);
    BASS_SetConfig(BASS_CONFIG_NET_READTIMEOUT, 10000);

    const float scaleFactor = CAEAudioHardware::GetMusicMasterScalingFactor();
    if (std::isfinite(scaleFactor))
    {
        CAudioStream::m_lastGlobalVolume = static_cast<std::uint32_t>(std::clamp(scaleFactor, 0.0f, 1.0f) * 7000.0f);
    }

    CAudioStream::m_lastAudioStreamUpdateTick = CNecessary::GetTickCount();
    CAudioStream::m_bInitialized = true;
}

void CAudioStream::StreamThread()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;)
    {
        m_requestChanged.wait(lock, [this] { return m_stopping || !CAudioStream::m_audioStreamUrl.empty(); });
        if (m_stopping) {
            return;
        }

        const auto request = m_request;
        std::string url = std::exchange(CAudioStream::m_audioStreamUrl, std::string{});
        m_workerBusy = true;
        m_connecting = true;
        lock.unlock();

        const auto stream = BASS_StreamCreateURL(url.c_str(), 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS, nullptr, this);
        const int createError = stream ? 0 : BASS_ErrorGetCode();

        lock.lock();
        m_connecting = false;
        m_cancelPending = false;

        if (request == m_request && !m_stopping)
        {
            if (stream)
            {
                {
                    const std::lock_guard<std::mutex> tagsLock(m_tagsMutex);
                    m_tagsChannel = stream;
                }

                BASS_ChannelSetSync(stream, BASS_SYNC_META, 0, &AudioStreamMetaSync, this);
                BASS_ChannelSetSync(stream, BASS_SYNC_OGG_CHANGE, 0, &AudioStreamMetaSync, this);
                if (BASS_ChannelLock(stream, true))
                {
                    CAudioStream::AudioStreamMetaSync(0, stream, 0, this);
                    BASS_ChannelLock(stream, false);
                }

                BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, CAudioStream::m_lastGlobalVolume / 10000.0f);
                if (BASS_ChannelPlay(stream, false))
                {
                    CAudioStream::bassStreamHandle = stream;
                    m_requestChanged.wait(lock, [this, request] { return m_stopping || request != m_request; });
                }
                else
                {
                    CNecessary::Log(OBFUSCATE("CAudioStream: BASS_ChannelPlay failed: %d"), BASS_ErrorGetCode());
                    CAudioStream::m_bIsPlaying = false;
                }
            }
            else
            {
                CNecessary::Log(OBFUSCATE("CAudioStream: BASS_StreamCreateURL failed: %d"), createError);
                CAudioStream::m_bIsPlaying = false;
            }
        }

        if (stream)
        {
            {
                const std::lock_guard<std::mutex> tagsLock(m_tagsMutex);
                m_tagsChannel = 0;
                CAudioStream::m_streamTitle[0] = '\0';
            }

            lock.unlock();
            BASS_StreamFree(stream);
            lock.lock();
        }

        m_workerBusy = false;
        m_requestChanged.notify_all();
    }
}

void CAudioStream::Play(const char* streamUrl, float posX, float posY, float posZ, float streamRadius, bool isPositionBased)
{
    if (!CAudioStream::m_bInitialized || !streamUrl || !*streamUrl) {
        return;
    }

    if (strncasecmp(streamUrl, "http://", 7) != 0 && strncasecmp(streamUrl, "https://", 8) != 0 && strncasecmp(streamUrl, "ftp://", 6) != 0) {
        return;
    }

    if (isPositionBased && (!std::isfinite(posX) || !std::isfinite(posY) || !std::isfinite(posZ) || !std::isfinite(streamRadius) || streamRadius <= 0.0f)) {
        return;
    }

    CAudioStream::Stop(false);

    const float scaleFactor = CAEAudioHardware::GetMusicMasterScalingFactor();
    if (!std::isfinite(scaleFactor) || scaleFactor <= 0.0f) {
        return;
    }

    if (!CAudioStream::m_audioStreamThread.joinable())
    {
        try
        {
            CAudioStream::m_audioStreamThread = std::thread(&CAudioStream::StreamThread, this);
        }
        catch (const std::system_error& error)
        {
            CNecessary::Log(OBFUSCATE("CAudioStream: cannot start stream thread: %s"), error.what());
            return;
        }
    }

    CAudioStream::m_audioStreamPosition = {posX, posY, posZ};
    CAudioStream::m_audioStreamRadius = streamRadius;
    CAudioStream::m_isAudioStreamPositionBased = isPositionBased;

    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        CAudioStream::m_audioStreamUrl = streamUrl;
        CAudioStream::m_bIsPlaying = true;
    }

    m_requestChanged.notify_one();

    if (!CAudioStream::m_bAudioMsg && CGui::GetChat())
    {
        CGui::GetChat()->AddInfoMessage(OBFUSCATE("Audio Stream: %s"), streamUrl);
    }
}

void CAudioStream::Stop(bool shouldWaitForThread)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_request;
    CAudioStream::m_audioStreamUrl.clear();
    CAudioStream::m_bIsPlaying = false;

    {
        const std::lock_guard<std::mutex> tagsLock(m_tagsMutex);
        m_tagsChannel = 0;
        CAudioStream::m_streamTitle[0] = '\0';
    }

    CAudioStream::m_streamLabel[0] = '\0';

    if (CAudioStream::bassStreamHandle)
    {
        BASS_ChannelStop(std::exchange(CAudioStream::bassStreamHandle, 0));
    }

    m_cancelPending = m_connecting;
    CAudioStream::CancelConnection();
    m_requestChanged.notify_one();

    while (shouldWaitForThread && m_workerBusy)
    {
        if (m_cancelPending)
        {
            m_requestChanged.wait_for(lock, std::chrono::milliseconds(10));
            CAudioStream::CancelConnection();
        }
        else
        {
            m_requestChanged.wait(lock);
        }
    }
}

void CAudioStream::CancelConnection()
{
    if (m_cancelPending && BASS_StreamCancel(this))
    {
        m_cancelPending = false;
    }
}

void CAudioStream::Process()
{
    const auto now = static_cast<std::uint32_t>(CNecessary::GetTickCount());
    bool ended;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        CAudioStream::CancelConnection();
        if (!CAudioStream::m_bIsPlaying || now - CAudioStream::m_lastAudioStreamUpdateTick <= 100) {
            return;
        }

        ended = CAudioStream::bassStreamHandle && BASS_ChannelIsActive(CAudioStream::bassStreamHandle) == BASS_ACTIVE_STOPPED;
    }

    CAudioStream::m_lastAudioStreamUpdateTick = now;

    if (ended)
    {
        CAudioStream::Stop(false);
        return;
    }

    const float musicScale = CAEAudioHardware::GetMusicMasterScalingFactor();
    const float scaleFactor = std::isfinite(musicScale) ? std::clamp(musicScale, 0.0f, 1.0f) : 0.0f;
    std::uint32_t volume = static_cast<std::uint32_t>(scaleFactor * 7000.0f);
    if (CGame::IsMenuActive())
    {
        volume = 0;
    }
    else if (CAudioStream::m_isAudioStreamPositionBased)
    {
        if (CGame::GetPlayer() && CGame::GetPlayer()->VerifyInstance())
        {
            const float distance = CGame::GetPlayer()->GetDistanceFromPoint(CAudioStream::m_audioStreamPosition.x, CAudioStream::m_audioStreamPosition.y, CAudioStream::m_audioStreamPosition.z);
            if (std::isfinite(distance) && distance >= 0.0f && distance <= CAudioStream::m_audioStreamRadius)
            {
                const float fadeFactor = 1.0f - (distance / CAudioStream::m_audioStreamRadius);
                volume = static_cast<std::uint32_t>(fadeFactor * (scaleFactor * 8000.0f));
            }
            else
            {
                volume = 0;
            }
        }
    }

    CAudioStream::SetVolume(volume);
    CAudioStream::UpdateStreamTags();
}

void CAudioStream::ControlGameRadio()
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (!CAudioStream::m_bIsPlaying) {
            return;
        }
    }

    CAudioEngine::StartRadio(-1);
    CAudioEngine::StopRadio();
}

void CAudioStream::SetVolume(std::uint32_t volume)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    CAudioStream::m_lastGlobalVolume = volume;
    if (CAudioStream::bassStreamHandle)
    {
        BASS_ChannelSetAttribute(CAudioStream::bassStreamHandle, BASS_ATTRIB_VOL, volume / 10000.0f);
    }
}

void CAudioStream::AudioStreamMetaSync(HSYNC, std::uint32_t streamChannel, std::uint32_t, void* userData)
{
    auto* audioStream = static_cast<CAudioStream*>(userData);
    char title[sizeof(audioStream->m_streamTitle)]{};
    if (const auto* meta = BASS_ChannelGetTags(streamChannel, BASS_TAG_META))
    {
        if (const auto* start = std::strstr(meta, "StreamTitle='"))
        {
            start += sizeof("StreamTitle='") - 1;
            if (const auto* end = std::strstr(start, "';"))
            {
                const auto length = std::min<std::size_t>(end - start, sizeof(title) - 1);
                std::memcpy(title, start, length);
            }
        }
    }
    else if (const auto* ogg = BASS_ChannelGetTags(streamChannel, BASS_TAG_OGG))
    {
        const char* artist = nullptr;
        const char* track = nullptr;
        for (const char* entry = ogg; *entry; entry += CNecessary::MPSTRLEN(entry) + 1)
        {
            if (std::strncmp(entry, OBFUSCATE("artist="), 7) == 0)
            {
                artist = entry + 7;
            }

            if (std::strncmp(entry, OBFUSCATE("title="), 6) == 0)
            {
                track = entry + 6;
            }
        }

        if (track && *track)
        {
            if (artist && *artist)
            {
                std::snprintf(title, sizeof(title), "%s - %s", artist, track);
            }
            else
            {
                std::snprintf(title, sizeof(title), "%s", track);
            }
        }
    }

    const std::lock_guard<std::mutex> lock(audioStream->m_tagsMutex);
    if (streamChannel == audioStream->m_tagsChannel)
    {
        std::memcpy(audioStream->m_streamTitle, title, sizeof(title));
    }
}

void CAudioStream::UpdateStreamTags()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (!CAudioStream::bassStreamHandle) {
        return;
    }

    char title[sizeof(CAudioStream::m_streamTitle)];
    {
        const std::lock_guard<std::mutex> tagsLock(m_tagsMutex);
        std::memcpy(title, CAudioStream::m_streamTitle, sizeof(title));
    }

    if (!*title)
    {
        CAudioStream::m_streamLabel[0] = '\0';
        return;
    }

    if (!BASS_ChannelLock(CAudioStream::bassStreamHandle, true)) {
        return;
    }

    char label[sizeof(CAudioStream::m_streamLabel)]{};
    const auto* headers = BASS_ChannelGetTags(CAudioStream::bassStreamHandle, BASS_TAG_ICY);
    if (!headers) {
        headers = BASS_ChannelGetTags(CAudioStream::bassStreamHandle, BASS_TAG_HTTP);
    }

    if (headers)
    {
        for (const char* entry = headers; *entry; entry += CNecessary::MPSTRLEN(entry) + 1)
        {
            if (std::strncmp(entry, OBFUSCATE("icy-name:"), 9) == 0 && entry[9])
            {
                std::snprintf(label, sizeof(label), "%s - %s", title, entry + 9);
            }
        }
    }

    BASS_ChannelLock(CAudioStream::bassStreamHandle, false);
    CNecessary::ConvertLocalizedEncodingToUTF8(CAudioStream::m_streamLabel, label, sizeof(CAudioStream::m_streamLabel));
}

bool CAudioStream::GetStreamTags() const
{
    return CAudioStream::m_streamLabel[0] != '\0';
}

void CAudioStream::SetAudioMsg(bool toggle)
{
    CAudioStream::m_bAudioMsg = toggle;
}