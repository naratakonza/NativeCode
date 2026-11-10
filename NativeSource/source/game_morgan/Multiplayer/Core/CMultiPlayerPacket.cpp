#include "../../../../Main.h"
#include "CMultiPlayerPacket.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"
#include "../../Enums/eMaxItem.h"

#include <raknet/BitStream.h>
#include <raknet/SAMP/SAMPRPC.h>

void CMultiPlayerPacket::SendChatCommand(const char* szCommand)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !network->GetRakClient() || network->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED || !szCommand) {
        return;
    }

    const auto length = CNecessary::MPSTRLEN(szCommand);
    if (!length || length > eMaxItem::MAX_MESSAGE_LENGTH) {
        return;
    }

    RakNet::BitStream bsParams;
    const int iStrlen = static_cast<int>(length);
    bsParams.Write(iStrlen);
    bsParams.Write(szCommand, iStrlen);
    network->GetRakClient()->RPC(&RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CMultiPlayerPacket::SendChatMessage(const char* szMsg)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !network->GetRakClient() || network->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED || !szMsg) {
        return;
    }

    const auto length = CNecessary::MPSTRLEN(szMsg);
    if (!length || length > eMaxItem::MAX_MESSAGE_LENGTH) {
        return;
    }

    RakNet::BitStream bsSend;
    const auto byteTextLen = static_cast<uint8_t>(length);
    bsSend.Write(byteTextLen);
    bsSend.Write(szMsg, byteTextLen);
    network->GetRakClient()->RPC(&RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}
