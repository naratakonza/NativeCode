#include "../../../Main.h"
#include "CNetGame.h"
#include "CMultiPlayer.h"
#include "Guis/CGui.h"
#include "../CGame.h"
#include "../CClock.h"
#include "../CWeather.h"
#include "../CTheZones.h"
#include "../CLoadingScreen.h"
#include "../Base/CPad.h"
#include "../Scripts/CScripting.h"
// #include "Core/CMultiPlayerNetwork.h"

CNetGame::CNetGame(const char* szHostOrIp, int iPort, const char* szPlayerName, const char* szPass, int Size, bool hasChanged) : NetworkSettings{}
{
    CNecessary::Log(OBFUSCATE("NetGame Created %s %d"), szHostOrIp, iPort);

    memset(&m_networkData, 0, sizeof(NetworkData));
    strncpy(GetNetworkData().HostName, OBFUSCATE("San Andreas Multiplayer"), sizeof(GetNetworkData().HostName) - 1);
    strncpy(GetNetworkData().PlayerName, szPlayerName, sizeof(GetNetworkData().PlayerName) - 1);
    strncpy(GetNetworkData().ServerPassword, szPass, sizeof(GetNetworkData().ServerPassword) - 1);

    struct hostent* he = gethostbyname(szHostOrIp);
    if (he && he->h_addrtype == AF_INET && he->h_length == static_cast<int>(sizeof(in_addr)) && he->h_addr_list && he->h_addr_list[0])
    {
        inet_ntop(AF_INET, he->h_addr_list[0], GetNetworkData().HostOrIP, sizeof(GetNetworkData().HostOrIP));
        GetNetworkData().HostOrIP[sizeof(GetNetworkData().HostOrIP) - 1] = '\0';
    }
    else
    {
        CNecessary::Log(OBFUSCATE("Error: Unable to resolve host name"));
        strncpy(GetNetworkData().HostOrIP, szHostOrIp, sizeof(GetNetworkData().HostOrIP) - 1);
    }

    strncpy(GetNetworkData().HostDomain, szHostOrIp, sizeof(GetNetworkData().HostDomain) - 1);
    GetNetworkData().Port = iPort;
    GetNetworkData().Size = Size;

    CNetGame::InitialiseNetLogic();
    CNetGame::InitialiseRakClient();
    CNetGame::InitialisePools();
    CNetGame::InitialiseGameLogic();

    CClock::EnableGameClock(false);
    CTheZones::EnableZoneNames(false);

    CNetGame::m_dwLastConnectAttempt = CNecessary::GetTickCount();
    CNetGame::SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);

    CLoadingScreen::UpdatePercent(100);

    if (CGui::GetChat() && !hasChanged)
    {
        const auto sampVersion = SAMP_VERSION;
        CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("{FFFFFF}SA-MP {B9C9BF}%s {FFFFFF}Started"), static_cast<const char*>(sampVersion));
    }
}

CNetGame::~CNetGame()
{
    CNecessary::Log(OBFUSCATE("CNetGame Uninitialise"));
    // CNetwork::OnRaknetDisconnect();
    CNetGame::UninitialisePools();
    CNetGame::UninitialiseRakClient();
    memset(&m_networkData, 0, sizeof(NetworkData));
}

void CNetGame::InitialiseNetLogic()
{
    m_iNetModeNormalOnfootSendRate = NETMODE_ONFOOT_SENDRATE;
	m_iNetModeNormalInCarSendRate = NETMODE_INCAR_SENDRATE;
	m_iNetModeFiringSendRate = NETMODE_FIRING_SENDRATE;
	m_iNetModeSendMultiplier = NETMODE_SEND_MULTIPLIER;
}

void CNetGame::InitialiseRakClient()
{
    CNetGame::m_pRakClient = RakNetworkFactory::GetRakClientInterface();
    if (CNetGame::GetRakClient())
    {
        CNetGame::GetRakClient()->SetPassword(GetNetworkData().ServerPassword);
        CNetGame::GetRakClient()->SetMTUSize(GetNetworkData().Size);
    }
}

void CNetGame::InitialisePools()
{
    // CNetGame::m_pNetGamePools = new CNetGamePool();
}

void CNetGame::InitialiseGameLogic(bool eJect)
{
    if (eJect)
    {
        if (CMultiPlayer::GetActiveNetwork()->m_bManualVehicleEngineAndLight)
        {
            // CPatches::InstallVehicleEngineLightPatches();
        }

        fWorldBoundaries[0] = 20000.0f;
        fWorldBoundaries[1] = -20000.0f;
        fWorldBoundaries[2] = 20000.0f;
        fWorldBoundaries[3] = -20000.0f;
        return;
    }

    m_iSpawnsAvailable = 0;
	m_bHoldTime = true;
	m_byteWorldMinute = 0;
	m_byteWorldTime = 12;
	m_byteWeather =	1;
	m_fGravity = (float)0.0080000004;
	m_bUseCJWalk = false;
	m_bDisableEnterExits = false;
	m_fNameTagDrawDistance = 60.0f;
	m_bZoneNames = false;
	m_iVehicleFriendlyFire = true;
    m_bFriendlyFire = true;
	m_bInstagib = false;
	m_iDeathDropMoney = 0;
	m_bNameTagLOS = false;
    m_bUpdateCameraTarget = false;

	m_bLanMode = false;
    m_bDisableVehicleCollisions = false;
}

void CNetGame::UninitialiseRakClient()
{
	if (CNetGame::GetRakClient())
	{
		CNetGame::GetRakClient()->Disconnect(0);
		RakNetworkFactory::DestroyRakClientInterface(CNetGame::m_pRakClient);
		CNetGame::m_pRakClient = nullptr;
	}
}

void CNetGame::UninitialisePools()
{
    // SAFE_DELETE(CNetGame::m_pNetGamePools);
}

void CNetGame::Process()
{
    if (!CNetGame::GetRakClient() /*|| !CNetGame::GetNetGamePools()*/) {
		return;
	}

    // CMultiPlayerNetwork::Update();

    if (CMultiPlayer::GetActiveNetwork()->m_bHoldTime)
    {
        CClock::SetGameTime(CMultiPlayer::GetActiveNetwork()->m_byteWorldTime, CMultiPlayer::GetActiveNetwork()->m_byteWorldMinute);
    }

    CGame::LoadAnimationsAndModels();

    if (CNetGame::GetNetworkState() == eNetworkState::GAMESTATE_WAIT_CONNECT && (CNecessary::GetTickCount() - CNetGame::m_dwLastConnectAttempt) > 1000)
    {
        if (CGui::GetChat())
        {
            CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Connecting to %s:%d..."), GetNetworkData().HostDomain, GetNetworkData().Port);
        }

        if (CNetGame::GetRakClient())
        {
            CNetGame::GetRakClient()->Connect(GetNetworkData().HostOrIP, GetNetworkData().Port, 0, 0, 2);
        }

        CNetGame::m_dwLastConnectAttempt = CNecessary::GetTickCount();
        CNetGame::SetNetworkState(eNetworkState::GAMESTATE_CONNECTING);
    }

    if (CNetGame::GetNetworkState() == eNetworkState::GAMESTATE_CONNECTED)
	{
        CNetGame::ProcessPools();
    }
    else
    {
        // CNetGame::SpawnScreen();

        static bool once = false;
        if (once)
        {
            return;
        }
        else
        {
            once = true;
            CGame::GetCamera()->SetBehindPlayer();
		}
    }
}

void CNetGame::ProcessPools()
{
    // if (CNetGame::GetNetGamePools())
    // {
	// 	CNetGame::GetNetGamePools()->Process();
	// }
}

void CNetGame::SetNetworkState(eNetworkState state)
{
    if (state == CNetGame::GetNetworkState()) {
		return;
	}

	CNetGame::m_iNetState = state;

    switch (state) {
        case eNetworkState::GAMESTATE_WAIT_CONNECT : {
            CNecessary::Log(OBFUSCATE("Waiting for connection.."));
			break;
		}

		case eNetworkState::GAMESTATE_CONNECTING : {
            CNecessary::Log(OBFUSCATE("Connecting to server.."));
			break;
		}

		case eNetworkState::GAMESTATE_AWAIT_JOIN : {
            CNecessary::Log(OBFUSCATE("Connected. Joining the game.."));
			break;
		}

		case eNetworkState::GAMESTATE_CONNECTED : {
            CNecessary::Log(OBFUSCATE("Loading..."));
			break;
		}

		case eNetworkState::GAMESTATE_RESTARTING : {
            CNecessary::Log(OBFUSCATE("The server is restarting.."));

            if (CGui::GetChat()) {
                CGui::GetChat()->AddInfoMessage(OBFUSCATE("The server is restarting.."));
            }

			break;
		}

        case eNetworkState::GAMESTATE_NETWORKCHANGED : {
            CNecessary::Log(OBFUSCATE("Network data has changed.."));

            if (CGui::GetChat()) {
                CGui::GetChat()->AddInfoMessage(OBFUSCATE("Network data has changed"));
            }

			break;
		}

		case eNetworkState::GAMESTATE_NONE : {
            CNecessary::Log(OBFUSCATE("Didn't find any dick at all.."));
			break;
		}

		case eNetworkState::GAMESTATE_DISCONNECTED : {
            CNecessary::Log(OBFUSCATE("Server Disconnected.."));
			break;
		}
    }
}

void CNetGame::SpawnScreen()
{
    // if (!CGame::GetCamera() || !CGame::GetPlayer()) {
    if (!CGame::GetCamera()) {
        return;
    }

    CWeather::SetGameWeather(eWeatherType::WEATHER_SUNNY_LA);
    CGame::DisplayHUD(false);
    CPad::DisplayWidgets(false);

    // if (CGame::GetPlayer()->IsInVehicle())
    // {
    //     CGame::GetPlayer()->RemoveFromVehicleAndPutAt(1093.4, -2036.5, 82.710602);
    // }
    // else
    // {
    //     CGame::GetPlayer()->TeleportTo(1093.4f, -2036.5f, 82.7106f);
    // }

    // CGame::GetPlayer()->TeleportTo(1093.4f, -2036.5f, 82.7106f);

    CGame::GetCamera()->SetPosition(1093.0f, -2036.0f, 90.0f, 0.0f, 0.0f, 0.0f);
    CGame::GetCamera()->LookAtPoint(384.0f, -1557.0f, 20.0f, 2);
}
