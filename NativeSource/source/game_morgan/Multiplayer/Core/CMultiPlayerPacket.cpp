#include "../../../../Main.h"
#include "CMultiPlayerPacket.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"
#include "../../Enums/eMaxItem.h"

#include <raknet/BitStream.h>
#include <raknet/SAMP/SAMPRPC.h>

bool CMultiPlayerPacket::SendChatMessage(const char* text)
{
    return CMultiPlayerPacket::SendChatRPC(text, false);
}

bool CMultiPlayerPacket::SendChatCommand(const char* text)
{
    return CMultiPlayerPacket::SendChatRPC(text, true);
}

bool CMultiPlayerPacket::SendChatRPC(const char* text, bool command)
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !network->GetRakClient() || network->GetNetworkState() != GAMESTATE_CONNECTED || !text) {
        return false;
    }

    const auto length = std::strlen(text);
    if (length == 0 || length > eMaxItem::MAX_MESSAGE_LENGTH) {
        return false;
    }

    RakNet::BitStream stream;
    if (command) {
        stream.Write(static_cast<uint32_t>(length));
    } else {
        stream.Write(static_cast<uint8_t>(length));
    }
    stream.Write(text, static_cast<int>(length));
    return network->GetRakClient()->RPC(command ? &RPC_ServerCommand : &RPC_Chat, &stream, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}
