#include "../../../../Main.h"
#include "CMultiPlayerNetwork.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"
#include "../Guis/CGui.h"
#include <raknet/SAMP/SAMPRPC.h>
#include <ctime>

void gen_auth_key(char buf[260], char* auth_in);

unsigned char CMultiPlayerNetwork::GetPacketID(Packet *p)
{
	// CNecessary::Log(OBFUSCATE("CMultiPlayerNetwork GetPacketID %d"), p);

	if (p == 0) {
		return 255;
	}

	if ((unsigned char)p->data[0] == ID_TIMESTAMP) {
		return (unsigned char)p->data[sizeof(unsigned char) + sizeof(RakNetTime)];
	}

	return (unsigned char)p->data[0];
}

void CMultiPlayerNetwork::Packet_AuthKey(Packet* pkt)
{
	RakNet::BitStream bsAuth((unsigned char *)pkt->data, pkt->length, false);

	uint8_t byteAuthLen;
	char szAuth[260] = { '\0' };

	bsAuth.IgnoreBits(8);
	bsAuth.Read(byteAuthLen);
	bsAuth.Read(szAuth, byteAuthLen);
	szAuth[byteAuthLen] = '\0';

	char szAuthKey[260] = { '\0' };
	gen_auth_key(szAuthKey, szAuth);

	RakNet::BitStream bsKey;
	auto byteAuthKeyLen = (uint8_t)CNecessary::MPSTRLEN(szAuthKey);

	bsKey.Write((uint8_t)ID_AUTH_KEY);
	bsKey.Write((uint8_t)byteAuthKeyLen);
	bsKey.Write(szAuthKey, byteAuthKeyLen);
	bsKey.Write(true);
	
	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, 0);
	}

    // CNecessary::Log(OBFUSCATE("[AUTH] %s -> %s"), szAuth, szAuthKey);
}

void CMultiPlayerNetwork::Packet_ConnectionSucceeded(Packet* pkt)
{
	if (!CMultiPlayer::GetActiveNetwork()) {
		return;
	}

    if (CGui::GetChat())
	{
		CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Connected. Joining the game..."));
	}

    CNecessary::Log(OBFUSCATE("Connected. Joining the game..."));

	CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_AWAIT_JOIN);

	RakNet::BitStream bsSuccAuth((unsigned char *)pkt->data, pkt->length, false);
	uint16_t MyPlayerID;
	unsigned int uiChallenge;

	bsSuccAuth.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
	bsSuccAuth.IgnoreBits(32); // binaryAddress
	bsSuccAuth.IgnoreBits(16); // port
	bsSuccAuth.Read(MyPlayerID);
	bsSuccAuth.Read(uiChallenge);

	int iVersion = NETGAME_VERSION;
	uint8_t byteMod = 1;
	unsigned int uiClientChallengeResponse = uiChallenge ^ iVersion;

	char byteAuthBSLen = (char)CNecessary::MPSTRLEN(AUTH_BS_TH);
	char byteNameLen = (char)CNecessary::MPSTRLEN(CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName);

	const char* szSampVersion;

	if (strcmp(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostDomain, OBFUSCATE("graj.samp.gg")) == 0 || strcmp(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostDomain, OBFUSCATE("ip.blood.ovh")) == 0)
	{
		szSampVersion = OBFUSCATE("thd445599");
	}
	else if (strcmp(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostDomain, OBFUSCATE("gta-irz.com")) == 0)
	{
		szSampVersion = OBFUSCATE("5.4.1");
	}
	else if (strcmp(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostDomain, OBFUSCATE("morgan-games.com")) == 0 || strcmp(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostDomain, OBFUSCATE("168.222.20.137")) == 0)
	{
		szSampVersion = OBFUSCATE("MG1.0");
	}
	else
	{
		szSampVersion = SAMP_VERSION;
	}

	char byteClientverLen = (char)CNecessary::MPSTRLEN(szSampVersion);

	RakNet::BitStream bsSend;
	bsSend.Write(iVersion);
	bsSend.Write(byteMod);
	bsSend.Write(byteNameLen);
	bsSend.Write(CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName, byteNameLen);
	bsSend.Write(uiClientChallengeResponse);
	bsSend.Write(byteAuthBSLen);
	bsSend.Write(AUTH_BS_TH, byteAuthBSLen);
	bsSend.Write(byteClientverLen);
	bsSend.Write(szSampVersion, byteClientverLen);

	// CNetwork::OnRaknetConnect(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostOrIP, CMultiPlayer::GetActiveNetwork()->GetNetworkData().Port);
	// CNetwork::OnRaknetRpc(RPC_ClientJoin, bsSend);

	CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CMultiPlayerNetwork::Update()
{
    // CNecessary::Log(OBFUSCATE("CMultiPlayerNetwork Update"));

	if (!CMultiPlayer::GetActiveNetwork()) {
		return;
	}
	
	[[maybe_unused]] bool breakStatus = false;
	Packet* pkt = (Packet*)nullptr;
	unsigned char packetIdentifier = 0xFF;

	while ((pkt = CMultiPlayer::GetActiveNetwork()->GetRakClient()->Receive()))
	{
		packetIdentifier = CMultiPlayerNetwork::GetPacketID(pkt);

		switch (packetIdentifier)
		{
            default: {
                // CNecessary::Log(OBFUSCATE("Incoming packet is unknown! ID: %d"), packetIdentifier);
				break;
			}

            case PacketEnumeration::ID_AUTH_KEY: {
                // CNecessary::Log(OBFUSCATE("Incoming packet: ID_AUTH_KEY"));
                CMultiPlayerNetwork::Packet_AuthKey(pkt);
                break;
            }

            case PacketEnumeration::ID_CONNECTION_BANNED: {
                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("You are banned from this server."));
                }

                break;
            }

            case PacketEnumeration::ID_CONNECTION_ATTEMPT_FAILED: {
                CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);

                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("The server didn't respond. Retrying.."));
                }

                break;
            }

            case PacketEnumeration::ID_NO_FREE_INCOMING_CONNECTIONS: {
                CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);

                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("The server is full. Retrying..."));
                }

                break;
            }

            case PacketEnumeration::ID_DISCONNECTION_NOTIFICATION: {
				CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(2000);

				// CVoice::Enable(false);

				// CSpeaker::Hide();
				// CMicroPhone::Hide();

				// if (CAudio::GetAudioStream())
				// {
				// 	CAudio::GetAudioStream()->Stop();
				// }

                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Server closed the connection."));
                }

				
                break;
            }

            case PacketEnumeration::ID_CONNECTION_LOST: {
				CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(0);

                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Lost connection to the server. Reconnecting.."));
                }

				CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_RESTARTING);
				CMultiPlayer::Reconnect(false);
				// CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->DeleteAll();

				// if (CAudio::GetAudioStream())
				// {
				// 	CAudio::GetAudioStream()->Stop();
				// }
				
				CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);
				
                break;
            }

            case PacketEnumeration::ID_CONNECTION_REQUEST_ACCEPTED: {
                CMultiPlayerNetwork::Packet_ConnectionSucceeded(pkt);
                break;
            }

            case PacketEnumeration::ID_FAILED_INITIALIZE_ENCRIPTION: {
                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Failed to initialize encryption."));
                }

                break;
            }

            case PacketEnumeration::ID_INVALID_PASSWORD: {          
                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Wrong server password."));
                }

                CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(0);

                break;
            }
        }

		// if (!CNetwork::OnRaknetReceive(*pkt)) 
		// {
		// 	breakStatus = true;
		// }

		// if (breakStatus) {
		// 	return;
		// }

        CMultiPlayer::GetActiveNetwork()->GetRakClient()->DeallocatePacket(pkt);
    }
}