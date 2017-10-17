/**
 * @file
 */

#pragma once

#include "ClientMessages_generated.h"
#include "ClientNetwork.h"
#include <memory>

namespace network {

using namespace flatbuffers;

/**
 * Create and send messages from the client to the server.
 */
class ClientMessageSender {
private:
	ClientNetworkPtr _network;
public:
	ClientMessageSender(const ClientNetworkPtr& network);
	void sendClientMessage(FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef std::shared_ptr<ClientMessageSender> ClientMessageSenderPtr;

}
