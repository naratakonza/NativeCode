#include "../../../../Main.h"
#include "CMultiPlayerPacket.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"

void CMultiPlayerPacket::SendDialogResponse(uint16_t wDialogID, uint8_t byteButtonID, uint16_t wListBoxItem, char* szInput)
{
	if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetRakClient() || CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
		return;
	}

	if (wListBoxItem >= 65535)
    {
		wListBoxItem = -1;
	}

	uint8_t respLen = CNecessary::MPSTRLEN(szInput);

	RakNet::BitStream bsSend;
	bsSend.Write(wDialogID);
	bsSend.Write(byteButtonID);
	bsSend.Write(wListBoxItem);
	bsSend.Write(respLen);
	bsSend.Write(szInput, respLen);

	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_DialogResponses, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}
}

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