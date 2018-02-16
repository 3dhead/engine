/**
 * @file
 */

#include "World.h"
#include "network/ProtocolEnum.h"
#include "core/command/Command.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/entity/ai/AIRegistry.h"
#include "io/Filesystem.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/Common.h"
#include "LUAFunctions.h"
#include <SimpleAI.h>

namespace backend {

constexpr int aiDebugServerPort = 11338;
constexpr const char* aiDebugServerInterface = "127.0.0.1";

World::World(const MapProviderPtr& mapProvider, const AIRegistryPtr& registry,
		const core::EventBusPtr& eventBus, const io::FilesystemPtr& filesystem) :
		_mapProvider(mapProvider), _registry(registry),
		_eventBus(eventBus), _filesystem(filesystem) {
}

void World::update(long dt) {
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		map->update(dt);
	}
	_aiServer->update(dt);
}

void World::construct() {
	core::Command::registerCommand("sv_maplist", [this] (const core::CmdArgs& args) {
		for (auto& e : _maps) {
			const MapPtr& map = e.second;
			Log::info("Map %s", map->idStr().c_str());
		}
	}).setHelp("List all maps");

	core::Command::registerCommand("sv_spawnnpc", [this] (const core::CmdArgs& args) {
		if (args.size() < 2) {
			Log::info("Usage: sv_spawnnpc <mapid> <npctype> [amount:default=1]");
			Log::info("entity types are:");
			for (const char **t = network::EnumNamesEntityType(); *t != nullptr; ++t) {
				Log::info(" - %s", *t);
			}
			return;
		}
		const MapId id = core::string::toInt(args[0]);
		auto i = _maps.find(id);
		if (i == _maps.end()) {
			Log::info("Could not find the specified map");
			return;
		}
		const MapPtr& map = i->second;
		auto type = network::getEnum<network::EntityType>(args[1].c_str(), network::EnumNamesEntityType());
		if (type == network::EntityType::NONE) {
			Log::error("Invalid entity type given");
			return;
		}
		const int amount = args.size() == 3 ? core::string::toInt(args[2]) : 1;
		map->spawnMgr()->spawn((network::EntityType)type, amount);
	}).setHelp("Spawns a given amount of npcs of a particular type on the specified map");
}

bool World::init() {
	_registry->init();

	if (!_mapProvider->init()) {
		Log::error("Failed to init the map provider");
		return false;
	}

	_aiServer = new ai::Server(*_registry, aiDebugServerPort, aiDebugServerInterface);
	if (_aiServer->start()) {
		Log::info("Start the ai debug server on %s:%i", aiDebugServerInterface, aiDebugServerPort);
	} else {
		Log::error("Could not start the ai debug server");
	}

	_maps = _mapProvider->worldMaps();
	if (_maps.empty()) {
		Log::error("Could not initialize any map");
		return false;
	}
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		_aiServer->addZone(map->zone());
	}

	lua::LUAType map = _lua.registerType("Map");
	map.addFunction("id", luaMapGetId);
	map.addFunction("__gc", luaMapGC);
	map.addFunction("__tostring", luaMapToString);

	_lua.registerGlobal("map", luaGetMap);

	const std::string& luaScript = _filesystem->load("world.lua");
	if (!_lua.load(luaScript)) {
		Log::error("Failed to load world lua script: %s", _lua.error().c_str());
		return false;
	}

	_lua.newGlobalData<World>("World", this);
	if (!_lua.execute("init")) {
		Log::error("Failed to init world lua script: %s", _lua.error().c_str());
		return false;
	}

	return true;
}

void World::shutdown() {
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		_aiServer->removeZone(map->zone());
	}
	_maps.clear();
	_mapProvider->shutdown();
	delete _aiServer;
	_aiServer = nullptr;
}

}
