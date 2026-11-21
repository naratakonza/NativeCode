#pragma once

#include "CLocalPlayer.h"

#include "../../Entity/Ped/CPlayerPed.h"
// #include "../../Entity/Vehicle/CVehicle.h"
#include "../../Enums/eMarkerType.h"
#include "../../Enums/eSpecialAction.h"
#include "../../Enums/eNetworkPlayerState.h"
#include "../../Enums/eNetworkUpdateType.h"
#include "../../Enums/eFightingStyle.h"
#include "../../Enums/eSpecialVehicle.h"
#include "../../Enums/eWeaponSampid.h"

#include "../Core/CMultiPlayerShot.h"

#define NETMODE_PLAYERS_UPDATE_DELAY 3000
#define SYNC_ANIM_VEND_EAT_P 0x1804067C

#pragma pack(push, 1)
struct PlayerData {
    char szPlayerName[24 + 1];
    uint16_t playerId;
    bool bIsNPC;
    bool bIsAFK;
    eSpecialAction byteSpecialAction;
    uint8_t byteTeam;
	bool bShowNameTag;
    uint32_t dwColor;
    uint32_t dwLocalMarkerID;
	uint32_t dwGlobalMarkerID;
	CPlayerPed* pGamePlayer;
	CVector vecTargetPos;
	CVector vecTargetSpeed;
	bool bCollisionChecking;
};
#pragma pack(pop)

// #pragma pack(push, 1)
// struct VehicleData
// {
// 	uint16_t 			vehicleId;
// 	uint8_t 			byteSeatId;
// 	CVehicle* 			pGameVehicle;
// 	CVector 			vecTargetPos;
// 	CVector 			vecTargetSpeed;
// 	CQuaternion			quatData;
// };
// #pragma pack(pop)

struct HeadData
{
	CVector m_vecHeadDir;
	uint32_t m_dwLastHeadUpdate;
	uint32_t m_dwLastHeadLook;
};

class CRemotePlayer
{
public:
	uint32_t m_dwChatBubbleColor = 0xFFFFFFFF;
	uint32_t m_dwChatBubbleLastTick = 0;
	uint32_t m_dwChatBubbleCurDelay = 0;
	bool m_bChatBubbleIsActive = false;
	float m_fChatBubbleDrawDistance = 0.0f;
	char m_szChatBubbleMessage[256]{};

	int Score, Ping;

private:
	PlayerData m_playerData{};
	// VehicleData m_vehicleData{};
	HeadData m_headData{};

	uint32_t m_dwLastReceiveSyncTick;
	uint32_t m_dwLastStoredSyncDataTime;

	float m_fReportedHealth;
    float m_fReportedArmour;

	int32_t m_usPlayingAnimIndex;
	bool m_bPerformingCustomAnimation;
	bool m_bPassengerDriveByMode;

    eNetworkPlayerState 	m_byteNetworkPlayerState;
	eNetworkUpdateType 		m_byteNetworkUpdateType;

	ONFOOT_SYNC_DATA		m_ofSyncData;
	// INCAR_SYNC_DATA			m_icSyncData;
	// PASSENGER_SYNC_DATA 	m_psSyncData;
	AIM_SYNC_DATA			m_aimSyncData;
	// BULLET_SYNC_DATA		m_bulletSyncData;
	// TRAILER_SYNC_DATA 		m_trailerSyncData;
	// UNOCCUPIED_SYNC_DATA 	m_unoccupiedSyncData;

private:
	CVector m_vecPosOffset;

public:
	CRemotePlayer(const char*, uint16_t, bool bIsNPC = false);
	~CRemotePlayer();

	void Process();
	void UpdateReceiveSyncTick();
	uint32_t GetReceiveSyncTick();
	void UpdateReceiveDataTimeSyncTick();
	uint32_t GetReceiveDataTimeSyncTick();
	void UpdateOnceRequiredSync();
	void UpdateAllFrames();
	void DoChatBubbleMessages();
	bool IsActive();
	void Deactivate();

	void ResetData();
    void ResetAll();
	void ResetAllSync();
	void UpdateAllDataTime();
	void ResetSpeedAndKeysForGamePlayer();
	// void SetGameVehicle(CVehicle*);
	// CVehicle* GetGameVehicle();
	void SetGamePlayer(CPlayerPed*);
	CPlayerPed* GetGamePlayer();
	bool IsGamePlayerValid(uint16_t id, CPlayerPed* pGamePlayer, bool bDebug = true);
    CPlayerPed* CreateGamePlayer(unsigned int, CVector*, float);
	void RemoveGamePlayer(uint8_t reset = 0, bool bValid = true);
    bool SpawnGamePlayer(uint8_t, unsigned int, CVector *, float, uint32_t dwColor, eFightingStyle byteFightingStyle, const uint16_t weaponSkills[11]);
	void HandleRemovingFromVehicle();
	void HandlePlayerSyncAnimation();

	void UpdatePlayerWeaponAim();

	void WritePlayerSyncData(ONFOOT_SYNC_DATA *syncData, uint32_t dwTime);
	void WriteVehicleSyncData(INCAR_SYNC_DATA *syncData, uint32_t dwTime);
	void WriteVehiclePassengerSyncData(PASSENGER_SYNC_DATA *syncData);
	void WriteTrailerSyncData(TRAILER_SYNC_DATA* syncData);
	void WriteUnoccupiedSyncData(UNOCCUPIED_SYNC_DATA* syncData);
	void WriteBulletSyncData(BULLET_SYNC_DATA* syncData);
	void WriteAimSyncData(AIM_SYNC_DATA* syncData);

	void UpdatePlayerTargetPosition();
	void UpdatePlayerPositionAndSpeed(CVector *vecPos, CVector *vecMove);
	void SetPlayerTargetPos(CVector pos);
	void SetPlayerTargetSpeed(CVector speed);
	void SetVehicleTargetPos(CVector pos);
	void SetVehicleTargetSpeed(CVector speed);
	void SetPlayerHeadDirectionPos(CVector direction);
	void UpdateGamePlayerRotation();
	void UpdateGameVehicleRotation();
	void UpdateGamePlayerWeapon(uint8_t, bool armed = false);
	void UpdateGamePassengerDriveBy();
	void UpdateGameCuffedStateBySeat();
	void UpdateGameVehicleData(uint16_t);
	void UpdateGameVehicleTrailer();
	void ProcessGamePlayerHead();
	void ProcessGameSurfing();
	void ProcessGameRCVehicle();
	void ProcessGamePlayerSpecialAction(eSpecialAction byteSpecialAction);
	void ProcessGamePlayerAnimation();

	void UpdateVehicleMatrixAndSpeed(CMatrix* mat, CVector* pos, CVector* speed);
	void SetVehicleQuatMatrix(CMatrix mat);
	void UpdateTrainMatrixAndSpeed(CMatrix* matWorld, CVector* vecMoveSpeed, float fTrainSpeed);
	void UpdateGameVehicleTargetPosition();
	void UpdateGameVehicleControls();
	void ClampTurnSpeedVectorForGameVehicle(CVector& vec);
	void NormalizeGameVehicleTurnSpeed();

	void SetGameVehicleMoveSpeed(CVector);
	void SetGameVehicleTurnSpeed(CVector);
	void SetGamePlayerMoveSpeed(CVector);
	void SetGamePlayerTurnSpeed(CVector);
	void SetCollisionCheckingState(bool);
	bool GetCollisionCheckingState();
	bool IsValidInstance();
	void RemoveGamePlayerFromVehicle();
	void HandleWasted();

	void HandleGameVehicleEntryExit();
	void EnterToGameVehicle(uint16_t, bool);
	void ExitFromGameVehicle();
	void SetVehicleSeatId(uint8_t);
	uint8_t GetVehicleSeatId();

    void SetRemotePlayerID(uint16_t);
	uint16_t GetRemotePlayerID();

	void SetRemoteVehicleID(uint16_t);
	uint16_t GetRemoteVehicleID();

    void SetRemoteNPC(bool);
	bool IsRemoteNPC();

	void SetRemoteAFK(bool);
	bool IsRemoteAFK();

	void SetRemoteShowNameTag(bool);
	bool IsRemoteNameTag();

    void SetRemoteName(const char*);
	char* GetRemoteName();

	void SetReportedHealth(float fHealth);
	void SetReportedArmour(float fArmour);
	float GetReportedHealth();
	float GetReportedArmour();

	bool IsSurfingOnVehicle();
	bool IsSurfingOnObject();

	void NetworkPlayerStateChange(uint8_t newState, uint8_t oldState);

	void SetNetworkUpdateType(eNetworkUpdateType);
	eNetworkUpdateType GetNetworkUpdateType();

    void SetNetworkPlayerState(eNetworkPlayerState);
	eNetworkPlayerState GetNetworkPlayerState();

	int GetRemoteScore() {
		return Score;
	};

	int GetRemotePing() {
		return Ping;
	};

	void SetRemotePing(int ping) {
		Ping = ping;
	};

	void SetRemoteScore(int score) {
		Score = score;
	};

	void SetRemoteTeam(uint8_t);
	uint8_t GetRemoteTeam();

	void SetRemotePassengerDriveBy(bool);
	bool GetRemotePassengerDriveBy();

	uint32_t GetRemoteColor();
	void SetRemoteColor(uint32_t dwColor);
	uint32_t GetPlayerColorAsARGB();
	void UpdateColorOnRadar(uint32_t dwColor);

	void CreateGlobalMarker(short sX, short sY, short sZ);
	void DestroyGlobalMarker();

	void SetMarkerID(eMarkerType, uint32_t);
	uint32_t GetGlobalMarkerID();
	uint32_t GetLocalMarkerID();

	void Say(unsigned char* szText);
	void AddBubbleMessage(uint32_t expireTime, const char* pText, uint32_t color, float drawDistance);

	void SetSpecialAction(eSpecialAction);
	eSpecialAction GetSpecialAction();

	void SetPlayingAnimIndex(int32_t);
	int32_t GetAnimPlayingIndex();

	void SetPerformingCustomAnimation(bool);
	bool IsPerformingCustomAnimation();

	void DoOnFootSync();
	void DoInCarSync();
	void DoPassengerSync();
	void DoOnFootFrame();
	void DoInCarFrame();
	void DoPassengerFrame();

	bool IsOnFootSync();
	bool IsInCarSync();
	bool IsPassengerSync();
	bool IsOnFootState();
	bool IsInCarState();
	bool IsPassengerState();

	float GetDistanceFromLocalPlayer();

};