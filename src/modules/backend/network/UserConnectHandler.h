/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "network/Network.h"
#include "core/TimeProvider.h"
#include "core/Log.h"
#include "ai/common/Types.h"

#include <flatbuffers/flatbuffers.h>

namespace backend {

/**
 * @see UserLogoutMgr
 */
class UserConnectHandler: public network::IProtocolHandler {
private:
	static constexpr auto logid = Log::logid("UserConnectHandler");
	network::NetworkPtr _network;
	MapProviderPtr _mapProvider;
	persistence::DBHandlerPtr _dbHandler;
	persistence::PersistenceMgrPtr _persistenceMgr;
	backend::EntityStoragePtr _entityStorage;
	network::ServerMessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	stock::StockProviderPtr _stockDataProvider;
	flatbuffers::FlatBufferBuilder _authFailed;

	void sendAuthFailed(ENetPeer* peer);
	UserPtr login(ENetPeer* peer, const std::string& email, const std::string& passwd);

public:
	UserConnectHandler(
			const network::NetworkPtr& network,
			const MapProviderPtr& mapProvider,
			const persistence::DBHandlerPtr& dbHandler,
			const persistence::PersistenceMgrPtr& persistenceMgr,
			const backend::EntityStoragePtr& entityStorage,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const stock::StockProviderPtr& stockDataProvider);

	void execute(ENetPeer* peer, const void* message) override;
};

}
