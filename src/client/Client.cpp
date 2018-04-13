/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "Client.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
#include "ui/turbobadger/FontUtil.h"
#include "ui/turbobadger/Window.h"
#include "core/command/Command.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "core/Password.h"
#include "network/IMsgProtocolHandler.h"
#include "network/AttribUpdateHandler.h"
#include "network/SeedHandler.h"
#include "network/AuthFailedHandler.h"
#include "network/EntityRemoveHandler.h"
#include "network/EntitySpawnHandler.h"
#include "network/EntityUpdateHandler.h"
#include "network/UserSpawnHandler.h"
#include "voxel/MaterialColor.h"
#include "core/Rest.h"

Client::Client(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const network::ClientNetworkPtr& network, const voxel::WorldMgrPtr& world, const network::ClientMessageSenderPtr& messageSender,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem) :
		Super(metric, filesystem, eventBus, timeProvider), _camera(), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_worldRenderer(world), _waiting(this) {
	_world->setClientData(true);
	init(ORGANISATION, "client");
}

Client::~Client() {
}

void Client::sendMovement() {
	if (_now - _lastMovement <= 100UL) {
		return;
	}

	network::MoveDirection moveMask = network::MoveDirection::NONE;
	if (_movement.left()) {
		moveMask |= network::MoveDirection::MOVELEFT;
	} else if (_movement.right()) {
		moveMask |= network::MoveDirection::MOVERIGHT;
	}
	if (_movement.forward()) {
		moveMask |= network::MoveDirection::MOVEFORWARD;
	} else if (_movement.backward()) {
		moveMask |= network::MoveDirection::MOVEBACKWARD;
	}

	if (_lastMoveMask == moveMask) {
		return;
	}
	_lastMovement = _now;
	_lastMoveMask = moveMask;
	// TODO: we can't use the camera, as we are aiming for a freelook mode, where the players' angles might be different from the camera's
	const float pitch = 0.0f;
	const float yaw = 0.0f;
	_messageSender->sendClientMessage(_moveFbb, network::ClientMsgType::Move, CreateMove(_moveFbb, moveMask, pitch, yaw).Union());
}

void Client::onEvent(const network::DisconnectEvent& event) {
	removeState(CLIENT_CONNECTING);
	ui::turbobadger::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const std::string& email = core::Var::getSafe(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::getSafe(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnect,
			network::CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(core::pwhash(password))).Union());
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, _dimension);
}

core::AppState Client::onConstruct() {
	core::AppState state = Super::onConstruct();

	_movement.construct();

	core::Var::get(cfg::ClientPort, SERVER_PORT);
	core::Var::get(cfg::ClientHost, SERVER_HOST);
	core::Var::get(cfg::ClientAutoLogin, "false");
	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "");
	core::Var::get(cfg::HTTPBaseURL, BASE_URL);
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "250.0");
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	_worldRenderer.construct();

	return state;
}

#define regHandler(type, handler, ...) \
	r->registerHandler(network::EnumNameServerMsgType(type), std::make_shared<handler>(__VA_ARGS__));

core::AppState Client::onInit() {
	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	eventBus()->subscribe<voxel::WorldCreatedEvent>(*this);

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	regHandler(network::ServerMsgType::AttribUpdate, AttribUpdateHandler);
	regHandler(network::ServerMsgType::EntitySpawn, EntitySpawnHandler);
	regHandler(network::ServerMsgType::EntityRemove, EntityRemoveHandler);
	regHandler(network::ServerMsgType::EntityUpdate, EntityUpdateHandler);
	regHandler(network::ServerMsgType::UserSpawn, UserSpawnHandler);
	regHandler(network::ServerMsgType::AuthFailed, AuthFailedHandler);
	regHandler(network::ServerMsgType::Seed, SeedHandler, _world, _eventBus);

	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_network->init()) {
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		return core::AppState::InitFailure;
	}

	_camera.init(glm::ivec2(0), dimension());
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTargetDistance(_maxTargetDistance->floatVal());
	_waiting.init();

	_meshPool->init();

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	if (!_world->init(filesystem()->load("worldparams.lua"), filesystem()->load("biomes.lua"))) {
		return core::AppState::InitFailure;
	}

	if (!_worldRenderer.init(glm::ivec2(0), _dimension)) {
		return core::AppState::InitFailure;
	}

	RestClient::init();

	_root.SetSkinBg(TBIDC("background"));
	_voxelFont.init("font.ttf", 14, 1, true, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ");

	handleLogin();

	return state;
}

void Client::handleLogin() {
	const core::VarPtr& autoLoginVar = core::Var::getSafe(cfg::ClientAutoLogin);
	if (autoLoginVar->boolVal()) {
		const int port = core::Var::getSafe(cfg::ClientPort)->intVal();
		const std::string& host = core::Var::getSafe(cfg::ClientHost)->strVal();
		Log::info("Trying to connect to server %s:%i", host.c_str(), port);
		if (!connect(port, host)) {
			autoLoginVar->setVal(false);
		}
	}

	if (!autoLoginVar->boolVal()) {
		new frontend::LoginWindow(this);
	}
}

void Client::beforeUI() {
	Super::beforeUI();

	if (_world->created()) {
		if (_player) {
			const glm::vec3& pos = _player->position();
			_camera.setTarget(pos);
		}
		_camera.setFarPlane(_worldRenderer.getViewDistance());
		_camera.init(glm::ivec2(0), dimension());
		_camera.update(_deltaFrameMillis);

		_drawCallsWorld = _worldRenderer.renderWorld(_camera);
		_drawCallsEntities = _worldRenderer.renderEntities(_camera);
		_worldRenderer.extractMeshes(_camera);
	} else {
		_drawCallsWorld = 0;
		_drawCallsEntities = 0;
	}
}

void Client::afterRootWidget() {
	const glm::vec3& pos = _camera.position();
	const glm::vec3& target = _camera.target();
	voxelfrontend::WorldRenderer::Stats stats;
	_worldRenderer.stats(stats);
	const int x = 5;
	enqueueShowStr(x, core::Color::White, "drawcalls world: %i", _drawCallsWorld);
	enqueueShowStr(x, core::Color::White, "drawcalls entities: %i", _drawCallsEntities);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	enqueueShowStr(x, core::Color::White, "pending: %i, meshes: %i, extracted: %i, uploaded: %i, visible: %i, octreesize: %i, octreeactive: %i, occluded: %i",
			stats.pending, stats.meshes, stats.extracted, stats.active, stats.visible, stats.octreeSize, stats.octreeActive, stats.occluded);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f (target: %.2f:%.2f:%.2f)", pos.x, pos.y, pos.z, target.x, target.y, target.z);

	if (hasState(CLIENT_CONNECTING)) {
		_waiting.render();
	}

	Super::afterRootWidget();
}

core::AppState Client::onCleanup() {
	eventBus()->unsubscribe<network::NewConnectionEvent>(*this);
	eventBus()->unsubscribe<network::DisconnectEvent>(*this);
	eventBus()->unsubscribe<voxel::WorldCreatedEvent>(*this);

	Log::info("shutting down the client");
	disconnect();
	_voxelFont.shutdown();
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	core::AppState state = Super::onCleanup();
	_world->shutdown();
	_player = frontend::ClientEntityPtr();
	_network->shutdown();
	_waiting.shutdown();
	_movement.shutdown();

	RestClient::disable();

	return state;
}

void Client::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, _maxTargetDistance->floatVal());
	_camera.setTargetDistance(targetDistance);
}

bool Client::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}

	if (key == SDLK_ESCAPE) {
		if (hasState(CLIENT_CONNECTING)) {
			removeState(CLIENT_CONNECTING);
			disconnect();
			_network->disconnect();
		}
	}

	return false;
}

core::AppState Client::onRunning() {
	_waiting.update(_deltaFrameMillis);
	core::AppState state = Super::onRunning();
	core::Var::visitBroadcast([] (const core::VarPtr& var) {
		Log::info("TODO: %s needs broadcast", var->name().c_str());
	});
	_movement.update(_deltaFrameMillis);
	_camera.rotate(glm::vec3(_mouseRelativePos.y, _mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
	_camera.update(_deltaFrameMillis);
	sendMovement();
	if (state == core::AppState::Running) {
		_network->update();
		if (_world->created()) {
			_worldRenderer.onRunning(_camera, _deltaFrameMillis);
		}
	}

	return state;
}

void Client::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(0), dimension());
}

void Client::signup(const std::string& email, const std::string& password) {
	const core::rest::Response& r = core::rest::post("signup",
			core::json { { "email", email }, { "password", core::pwhash(password) } });
	if (r.code != core::rest::StatusCode::OK) {
		Log::error("Failed to signup with %s (%i)", email.c_str(), r.code);
	}
}

void Client::lostPassword(const std::string& email) {
	const core::rest::Response& r = core::rest::post("lostpassword",
			core::json { { "email", email } });
	if (r.code != core::rest::StatusCode::OK) {
		Log::error("Failed to request the password reset for %s (%i)", email.c_str(), r.code);
	}
}

void Client::authFailed() {
	removeState(CLIENT_CONNECTING);
	core::Var::getSafe(cfg::ClientAutoLogin)->setVal(false);
	// TODO: stack (push/pop in UIApp) window support
	ui::turbobadger::Window* main = new frontend::LoginWindow(this);
	new frontend::AuthFailedWindow(main);
}

void Client::disconnect() {
	if (!_network->isConnected()) {
		return;
	}
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserDisconnect, network::CreateUserDisconnect(fbb).Union());
}

void Client::entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation) {
	const frontend::ClientEntityPtr& entity = _worldRenderer.getEntity(id);
	if (!entity) {
		Log::warn("Could not get entity with id %li", id);
		return;
	}
	entity->lerpPosition(pos, orientation);
}

void Client::entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos) {
	Log::info("Entity %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, (int)type);
	const std::string_view& meshName = "chr_skelett2_bake"; // core::string::toLower(network::EnumNameEntityType(type));
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	_worldRenderer.addEntity(std::make_shared<frontend::ClientEntity>(id, type, pos, orientation, mesh));
}

void Client::entityRemove(frontend::ClientEntityId id) {
	_worldRenderer.removeEntity(id);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation) {
	removeState(CLIENT_CONNECTING);
	Log::info("User %li (%s) logged in at pos %f:%f:%f with orientation: %f", id, name, pos.x, pos.y, pos.z, orientation);
	_camera.setTarget(pos);
	const video::MeshPtr& mesh = _meshPool->getMesh("chr_skelett2_bake");
	const network::EntityType type = network::EntityType::PLAYER;
	_player = std::make_shared<frontend::ClientEntity>(id, type, pos, orientation, mesh);
	_worldRenderer.addEntity(_player);
	_worldRenderer.extractMeshes(_camera);

	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnected,
			network::CreateUserConnected(fbb).Union());
}

bool Client::connect(uint16_t port, const std::string& hostname) {
	setState(CLIENT_CONNECTING);
	ENetPeer* peer = _network->connect(port, hostname);
	if (!peer) {
		removeState(CLIENT_CONNECTING);
		Log::error("Failed to connect to server %s:%i", hostname.c_str(), port);
		return false;
	}

	peer->data = this;
	Log::info("Connected to server %s:%i", hostname.c_str(), port);
	_waiting.setTextId("stateconnecting");
	return true;
}

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldMgrPtr& world = std::make_shared<voxel::WorldMgr>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::ClientNetworkPtr& network = std::make_shared<network::ClientNetwork>(protocolHandlerRegistry, eventBus);
	const network::ClientMessageSenderPtr& messageSender = std::make_shared<network::ClientMessageSender>(network);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	Client app(metric, meshPool, network, world, messageSender, eventBus, timeProvider, filesystem);
	return app.startMainLoop(argc, argv);
}
