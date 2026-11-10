#include "../../../Main.h"
#include "CNetGame.h"
#include "CMultiPlayer.h"
#include "Guis/CGui.h"
#include "../Enums/eMaxItem.h"
#include <raknet/BitStream.h>
#include <raknet/SAMP/SAMPRPC.h>
#include "../CGame.h"
#include "../CClock.h"
#include "../CWeather.h"
#include "../CTheZones.h"
#include "../CLoadingScreen.h"
#include "../Base/CPad.h"
#include "../Scripts/CScripting.h"
// #include "Core/CMultiPlayerNetwork.h"

void gen_auth_key(char buf[260], char* auth_in);

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
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_InitGame, CNetGame::InitGame);
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_ClientMessage, CNetGame::ClientMessage);
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_Chat, CNetGame::Chat);
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_ServerJoin, CNetGame::ServerJoin);
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_ServerQuit, CNetGame::ServerQuit);
        CNetGame::GetRakClient()->RegisterAsRemoteProcedureCall(&RPC_ConnectionRejected, CNetGame::ConnectionRejected);
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
    CNetGame::ProcessNetwork();

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
            if (CNetGame::GetRakClient()->Connect(GetNetworkData().HostOrIP, GetNetworkData().Port, 0, 0, 2)) {
                CNetGame::SetNetworkState(GAMESTATE_CONNECTING);
            } else {
                CNetGame::SetNetworkState(GAMESTATE_DISCONNECTED);
            }
        }

        CNetGame::m_dwLastConnectAttempt = CNecessary::GetTickCount();
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

void CNetGame::Reconnect()
{
    CNetGame::SetNetworkState(GAMESTATE_RESTARTING);
    CNetGame::UninitialiseRakClient();
    CNetGame::m_chatPlayers.clear();
    CNetGame::m_localPlayerId = 0xFFFF;
    CNetGame::InitialiseRakClient();
    CNetGame::m_dwLastConnectAttempt = CNecessary::GetTickCount();
    CNetGame::SetNetworkState(CNetGame::GetRakClient() ? GAMESTATE_WAIT_CONNECT : GAMESTATE_DISCONNECTED);
}

void CNetGame::ProcessNetwork()
{
    auto* client = CNetGame::GetRakClient();
    for (int index = 0; index < 128; ++index)
    {
        Packet* packet = client->Receive();
        if (!packet) {
            break;
        }

        if (packet->length && packet->data)
        {
            switch (packet->data[0])
            {
                case ID_AUTH_KEY:
                    CNetGame::Packet_AuthKey(packet);
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    CNetGame::Packet_ConnectionSucceeded(packet);
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                case ID_CONNECTION_LOST:
                case ID_CONNECTION_ATTEMPT_FAILED:
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                case ID_CONNECTION_BANNED:
                case ID_INVALID_PASSWORD:
                    CNetGame::m_chatPlayers.clear();
                    CNetGame::SetNetworkState(GAMESTATE_DISCONNECTED);
                    break;
                default:
                    break;
            }
        }
        client->DeallocatePacket(packet);
    }
}

void CNetGame::Packet_AuthKey(Packet* packet)
{
    if (CNetGame::GetNetworkState() != GAMESTATE_CONNECTING || packet->length < 2) {
        return;
    }

    const uint8_t length = packet->data[1];
    char challenge[260]{};
    if (!length || packet->length < 2u + length || std::memchr(packet->data + 2, '\0', length)) {
        return;
    }
    std::memcpy(challenge, packet->data + 2, length);
    char key[260]{};
    gen_auth_key(key, challenge);

    RakNet::BitStream stream;
    const auto keyLength = static_cast<uint8_t>(std::strlen(key));
    stream.Write(static_cast<uint8_t>(ID_AUTH_KEY));
    stream.Write(keyLength);
    stream.Write(key, keyLength);
    stream.Write(true);
    CNetGame::GetRakClient()->Send(&stream, SYSTEM_PRIORITY, RELIABLE, 0);
}

void CNetGame::Packet_ConnectionSucceeded(Packet* packet)
{
    if (CNetGame::GetNetworkState() != GAMESTATE_CONNECTING || packet->length < 13) {
        return;
    }

    RakNet::BitStream response(packet->data, packet->length, false);
    response.IgnoreBits(56);
    uint16_t playerId;
    uint32_t challenge;
    if (!response.Read(playerId) || !response.Read(challenge) || playerId >= eMaxItem::MAX_PLAYERS) {
        return;
    }

    constexpr int32_t netGameVersion = 4057;
    const auto auth = OBFUSCATE("39FB2DEEDB49ACFB8D4EECE6953D2507988CCCF4410");
    const auto version = SAMP_VERSION;
    const auto& data = CNetGame::GetNetworkData();
    const auto nameLength = static_cast<uint8_t>(std::strlen(data.PlayerName));
    const auto authLength = static_cast<uint8_t>(std::strlen(auth));
    const auto versionLength = static_cast<uint8_t>(std::strlen(version));

    RakNet::BitStream join;
    join.Write(netGameVersion);
    join.Write(static_cast<uint8_t>(1));
    join.Write(nameLength);
    join.Write(data.PlayerName, nameLength);
    join.Write(challenge ^ static_cast<uint32_t>(netGameVersion));
    join.Write(authLength);
    join.Write(auth, authLength);
    join.Write(versionLength);
    join.Write(version, versionLength);
    if (CNetGame::GetRakClient()->RPC(&RPC_ClientJoin, &join, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr))
    {
        CNetGame::m_localPlayerId = playerId;
        CNetGame::SetNetworkState(GAMESTATE_AWAIT_JOIN);
    }
}

void CNetGame::InitGame(RPCParameters* rpc)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || network->GetNetworkState() != GAMESTATE_AWAIT_JOIN || !rpc->input || rpc->numberOfBitsOfData < 120) {
        return;
    }

    RakNet::BitStream stream(rpc->input, (rpc->numberOfBitsOfData + 7) / 8, false);
    // Read the common 0.3.7 prefix up to the server-assigned player ID.
    stream.IgnoreBits(4 + 32 + 1 + 32 + 3 + 32);
    uint16_t playerId;
    if (stream.Read(playerId) && playerId == network->m_localPlayerId)
    {
        network->SetNetworkState(GAMESTATE_CONNECTED);
        if (CGui::GetChat()) {
            CGui::GetChat()->AddInfoMessage(OBFUSCATE("Connected. Chat is ready."));
        }
    }
}

void CNetGame::ClientMessage(RPCParameters* rpc)
{
    if (!CGui::GetChat() || !rpc->input || rpc->numberOfBitsOfData < 64) {
        return;
    }
    RakNet::BitStream stream(rpc->input, rpc->numberOfBitsOfData / 8, false);
    uint32_t color, length;
    char message[256]{};
    if (stream.Read(color) && stream.Read(length) && length > 0 && length < sizeof(message) && stream.Read(message, static_cast<int>(length))) {
        CGui::GetChat()->AddClientMessage(color, message);
    }
}

void CNetGame::Chat(RPCParameters* rpc)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !CGui::GetChat() || network->GetNetworkState() != GAMESTATE_CONNECTED || !rpc->input || rpc->numberOfBitsOfData < 24) {
        return;
    }
    RakNet::BitStream stream(rpc->input, rpc->numberOfBitsOfData / 8, false);
    uint16_t playerId;
    uint8_t length;
    char message[256]{};
    if (!stream.Read(playerId) || !stream.Read(length) || !length || playerId >= eMaxItem::MAX_PLAYERS || !stream.Read(message, length)) {
        return;
    }

    const auto player = network->m_chatPlayers.find(playerId);
    if (player != network->m_chatPlayers.end()) {
        CGui::GetChat()->AddChatMessage(player->second.name.c_str(), player->second.color, message);
    } else if (playerId == network->m_localPlayerId) {
        CGui::GetChat()->AddChatMessage(network->GetNetworkData().PlayerName, 0xFFFFFFFF, message);
    } else {
        char label[24];
        std::snprintf(label, sizeof(label), "Player %u", static_cast<unsigned>(playerId));
        CGui::GetChat()->AddChatMessage(label, 0xFFFFFFFF, message);
    }
}

void CNetGame::ServerJoin(RPCParameters* rpc)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !rpc->input || rpc->numberOfBitsOfData < 64) {
        return;
    }
    RakNet::BitStream stream(rpc->input, rpc->numberOfBitsOfData / 8, false);
    uint16_t playerId;
    uint32_t color;
    uint8_t npc, length;
    char name[eMaxItem::MAX_PLAYER_NAME + 1]{};
    if (stream.Read(playerId) && stream.Read(color) && stream.Read(npc) && stream.Read(length) &&
        playerId < eMaxItem::MAX_PLAYERS && length > 0 && length < sizeof(name) && stream.Read(name, length)) {
        network->m_chatPlayers[playerId] = {name, color};
    }
}

void CNetGame::ServerQuit(RPCParameters* rpc)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !rpc->input || rpc->numberOfBitsOfData < 16) {
        return;
    }
    RakNet::BitStream stream(rpc->input, rpc->numberOfBitsOfData / 8, false);
    uint16_t playerId;
    if (stream.Read(playerId)) {
        network->m_chatPlayers.erase(playerId);
    }
}

void CNetGame::ConnectionRejected(RPCParameters*)
{
    if (auto* network = CMultiPlayer::GetActiveNetwork()) {
        network->SetNetworkState(GAMESTATE_DISCONNECTED);
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
            if (CGui::GetChat()) {
                CGui::GetChat()->AddInfoMessage(OBFUSCATE("Server disconnected. Use /rec to reconnect."));
            }
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
