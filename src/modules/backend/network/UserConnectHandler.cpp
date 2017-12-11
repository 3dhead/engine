/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "UserConnectHandler.h"
#include "backend/entity/User.h"
#include "core/Var.h"
#include "core/Log.h"
#include "util/EMailValidator.h"
#include "UserModel.h"
#include "core/Password.h"
#include "backend/entity/EntityStorage.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"

namespace backend {

UserConnectHandler::UserConnectHandler(
		const network::NetworkPtr& network,
		const MapProviderPtr& mapProvider,
		const persistence::DBHandlerPtr& dbHandler,
		const backend::EntityStoragePtr& entityStorage,
		const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const stock::StockProviderPtr& stockDataProvider) :
		_network(network), _mapProvider(mapProvider), _dbHandler(dbHandler), _entityStorage(entityStorage),
		_messageSender(messageSender), _timeProvider(timeProvider), _containerProvider(containerProvider),
		_cooldownProvider(cooldownProvider), _stockDataProvider(stockDataProvider) {
	auto data = network::CreateAuthFailed(_authFailed);
	auto msg = network::CreateServerMessage(_authFailed, network::ServerMsgType::AuthFailed, data.Union());
	network::FinishServerMessageBuffer(_authFailed, msg);
}

void UserConnectHandler::sendAuthFailed(ENetPeer* peer) {
	ENetPacket* packet = enet_packet_create(_authFailed.GetBufferPointer(), _authFailed.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

UserPtr UserConnectHandler::login(ENetPeer* peer, const std::string& email, const std::string& passwd) {
	db::UserModel model;
	if (!_dbHandler->select(model, db::DBConditionUserModelEmail(email.c_str()))) {
		Log::warn("Could not get user id for email: %s", email.c_str());
		return UserPtr();
	}
	if (passwd != core::pwhash(model.password())) {
		return UserPtr();
	}
	const UserPtr& user = _entityStorage->user(model.id());
	if (user) {
		if (user->peer()->address.host == peer->address.host) {
			Log::debug("user %i reconnects with host %i on port %i", (int) model.id(), peer->address.host, peer->address.port);
			user->setPeer(peer);
			user->reconnect();
			return user;
		}
		Log::debug("skip connection attempt for client %i - the hosts don't match", (int) model.id());
		return UserPtr();
	}
	static const std::string name = "NONAME";
	MapPtr map = _mapProvider->map(model.mapid(), true);
	Log::info("user %i connects with host %i on port %i", (int) model.id(), peer->address.host, peer->address.port);
	const UserPtr& u = std::make_shared<User>(peer, model.id(), model.name(), map, _messageSender, _timeProvider,
			_containerProvider, _cooldownProvider, _dbHandler, _stockDataProvider);
	u->init();
	map->addUser(u);
	_entityStorage->addUser(u);
	return u;
}

void UserConnectHandler::execute(ENetPeer* peer, const void* raw) {
	const auto* message = getMsg<network::UserConnect>(raw);

	const std::string& email = message->email()->str();
	if (!util::isValidEmail(email)) {
		sendAuthFailed(peer);
		Log::debug("Invalid email given: '%s', %c", email.c_str(), email[0]);
		return;
	}
	const std::string& password = message->password()->str();
	if (password.empty()) {
		Log::debug("User tries to log into the gameserver without providing a password");
		sendAuthFailed(peer);
		return;
	}
	Log::info("User %s tries to log into the gameserver", email.c_str());

	const UserPtr& user = login(peer, email, password);
	if (!user) {
		sendAuthFailed(peer);
		return;
	}

	Log::info("User '%s' logged into the gameserver", email.c_str());
	const long seed = core::Var::getSafe(cfg::ServerSeed)->longVal();
	user->sendSeed(seed);
	user->sendUserSpawn();
}

}
