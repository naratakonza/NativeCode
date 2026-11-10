#pragma once

#include <cstdint>
#include "../Enums/eNetworkState.h"

//pools
// #include "Pools/CNetGamePool.h"

// raknet
#include <raknet/RakClientInterface.h>
#include <raknet/RakNetworkFactory.h>
#include <raknet/PacketEnumerations.h>
#include <raknet/StringCompressor.h>

#define NO_TEAM 255

#define NETMODE_HEADSYNC_SENDRATE		1000
#define NETMODE_AIM_SENDRATE			100
#define NETMODE_ONFOOT_SENDRATE	30
#define NETMODE_INCAR_SENDRATE	30
#define NETMODE_FIRING_SENDRATE 30
#define NETMODE_SEND_MULTIPLIER 2

typedef struct _NetworkData
{
	char HostName[0xFF];
	char PlayerName[24+1];
	char ServerPassword[0xFF];
	char HostOrIP[0xFF];
	char HostDomain[0xFF];
	int Port;
	int Size;
} NetworkData;

typedef struct _NetworkSettings
{
	bool		m_bZoneNames;
	bool		m_bUseCJWalk;
	bool		m_bAllowWeapons;
	bool		m_bLimitGlobalChatRadius;
	float		m_fGlobalChatRadius;
	float 		m_fNameTagDrawDistance;
	bool		m_bDisableEnterExits;
	bool		m_bNameTagLOS;
	bool		m_bManualVehicleEngineAndLight;
	uint32_t 	m_iSpawnsAvailable;
	bool 		m_bShowPlayerTags;
	uint32_t 	m_iShowPlayerMarkers;
	bool		m_bHoldTime;
	bool 		m_bHeadMove;
	uint8_t		m_byteWorldTime;
	uint8_t		m_byteWorldMinute;
	uint8_t		m_byteWeather;

	uint8_t* 	m_bChecked;

	float 		m_fGravity;
	uint32_t 	m_iDeathDropMoney;
	bool 		m_bInstagib;
	uint32_t 	m_iLagCompensation;
	uint32_t 	m_iVehicleFriendlyFire;
	bool		m_bFriendlyFire;
	bool		m_bLanMode;

	float 		fWorldBoundaries[4];
	bool		m_bDisableVehicleCollisions;
	bool		m_bUpdateCameraTarget;

	uint32_t 	m_iNetModeNormalOnfootSendRate;
	uint32_t 	m_iNetModeNormalInCarSendRate;
	uint32_t 	m_iNetModeFiringSendRate;
	uint32_t 	m_iNetModeSendMultiplier;
} NetworkSettings;

class CNetGame : public NetworkSettings
{
	friend class CMultiPlayer;
public:
	CNetGame(const char* szHostOrIp, int iPort, const char* szPlayerName, const char* szPass, int Size, bool hasChanged = false);
	~CNetGame();
	CNetGame(const CNetGame&) = delete;
	CNetGame& operator=(const CNetGame&) = delete;

	void InitialiseNetLogic();
	void InitialiseGameLogic(bool eJect = false);
	void SetNetworkState(eNetworkState state);
	void InitialiseRakClient();
	void UninitialiseRakClient();
	void InitialisePools();
	void UninitialisePools();
	void SpawnScreen();
	void Process();
	void ProcessPools();

public:
	NetworkData& GetNetworkData() {
		return CNetGame::m_networkData;
	};

	eNetworkState GetNetworkState() {
		return CNetGame::m_iNetState;
	};

	RakClientInterface* GetRakClient() { return m_pRakClient; };

	// CNetGamePool* GetNetGamePools() {
	// 	return CNetGame::m_pNetGamePools;
	// };

public:
	char m_szHostName[0xFF];
	char m_szHostOrIp[0x7F];
	int m_iPort;
	char m_szPassword[0xFF];
private:
	NetworkData m_networkData;
	RakClientInterface* m_pRakClient = nullptr;
	// CNetGamePool* m_pNetGamePools;
	eNetworkState m_iNetState = eNetworkState::GAMESTATE_NONE;
	uint32_t m_dwLastConnectAttempt = 0;
};