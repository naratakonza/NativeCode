#include "../../../../Main.h"
#include "CMultiPlayerPacket.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"

void CMultiPlayerPacket::SendChatCommand(const char* szCommand)
{
	if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetRakClient() || CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
		return;
	}

	RakNet::BitStream bsParams;
	int iStrlen = CNecessary::MPSTRLEN(szCommand);

	bsParams.Write(iStrlen);
	bsParams.Write(szCommand, iStrlen);

	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}
}

void CMultiPlayerPacket::SendChatMessage(const char* szMsg)
{
	if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetRakClient() || CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
		return;
	}

	RakNet::BitStream bsSend;
	uint8_t byteTextLen = CNecessary::MPSTRLEN(szMsg);

	bsSend.Write(byteTextLen);
	bsSend.Write(szMsg, byteTextLen);

	CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}