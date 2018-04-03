/**
 * @file
 */

#include "UserCooldownMgr.h"
#include "persistence/DBHandler.h"
#include "persistence/PersistenceMgr.h"
#include "core/Log.h"
#include "network/ServerMessageSender.h"
#include "backend/entity/User.h"

namespace backend {

static constexpr uint32_t FOURCC = FourCC('C','O','O','L');

UserCooldownMgr::UserCooldownMgr(User* user,
		const core::TimeProviderPtr& timeProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::DBHandlerPtr& dbHandler,
		const persistence::PersistenceMgrPtr& persistenceMgr) :
		Super(timeProvider, cooldownProvider), _dbHandler(dbHandler),
		_persistenceMgr(persistenceMgr), _user(user) {
}

bool UserCooldownMgr::init() {
	if (!_dbHandler->select(db::CooldownModel(), db::DBConditionCooldownModelUserid(_user->id()), [this] (db::CooldownModel&& model) {
		const int32_t id = model.cooldownid();
		const cooldown::Type type = (cooldown::Type)id;
		const uint64_t millis = model.starttime().millis();
		const cooldown::CooldownPtr& cooldown = createCooldown(type, millis);
		_cooldowns[type] = cooldown;
		if (cooldown->running()) {
			_queue.push(cooldown);
		}
	})) {
		Log::warn("Could not load cooldowns for user " PRIEntId, _user->id());
	}

	// initialize the models
	_dirtyModels.resize(int(cooldown::Type::MAX));
	for (std::underlying_type<cooldown::Type>::type i = 0; i < int(cooldown::Type::MAX); ++i) {
		db::CooldownModel& model = _dirtyModels[i];
		model.setCooldownid(i);
		model.setUserid(_user->id());
	}
	return _persistenceMgr->registerSavable(FOURCC, this);
}

void UserCooldownMgr::shutdown() {
	_persistenceMgr->unregisterSavable(FOURCC, this);
}

cooldown::CooldownTriggerState UserCooldownMgr::triggerCooldown(cooldown::Type type, cooldown::CooldownCallback callback) {
	return Super::triggerCooldown(type, [this, type, callback] (cooldown::CallbackType callbackType) {
		if (callback) {
			callback(callbackType);
		}
		sendCooldown(type, callbackType == cooldown::CallbackType::Started);
	});
}

void UserCooldownMgr::sendCooldown(cooldown::Type type, bool started) const {
	network::ServerMsgType msgtype;
	flatbuffers::Offset<void> msg;
	if (started) {
		const uint64_t duration = _cooldownProvider->duration(type);
		const uint64_t now = _timeProvider->tickMillis();
		msg = network::CreateStartCooldown(_cooldownFBB, type, now, duration).Union();
		msgtype = network::ServerMsgType::StartCooldown;
	} else {
		msg = network::CreateStopCooldown(_cooldownFBB, type).Union();
		msgtype = network::ServerMsgType::StopCooldown;
	}
	_user->sendMessage(_cooldownFBB, msgtype, msg);
}

bool UserCooldownMgr::getDirtyModels(Models& models) {
	// TODO: what about deleting...
	core::ScopedReadLock lock(_lock);
	models.reserve(models.size() + _cooldowns.size());
	for (const auto& e : _cooldowns) {
		const cooldown::CooldownPtr& c = e.second;
		db::CooldownModel& model = _dirtyModels[(int)c->type()];
		model.setStarttime(c->startMillis());
		models.push_back(&model);
	}
	return true;
}

}
