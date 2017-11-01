/**
 * @file
 */

#include "CooldownMgr.h"
#include "core/Common.h"
#include "core/Singleton.h"

namespace cooldown {

CooldownMgr::CooldownMgr(const core::TimeProviderPtr& timeProvider, const cooldown::CooldownProviderPtr& cooldownProvider) :
		_timeProvider(timeProvider), _cooldownProvider(cooldownProvider), _lock("CooldownMgr") {
}

CooldownTriggerState CooldownMgr::triggerCooldown(Type type, CooldownCallback callback) {
	core::ScopedWriteLock lock(_lock);
	CooldownPtr cooldown = _cooldowns[type];
	if (!cooldown) {
		// TODO: use a pool here
		cooldown = std::make_shared<Cooldown>(type, defaultDuration(type), callback, _timeProvider);
		_cooldowns[type] = cooldown;
	} else if (cooldown->running()) {
		Log::trace("Failed to trigger the cooldown of type %i: already running", std::enum_value(type));
		return CooldownTriggerState::ALREADY_RUNNING;
	}
	cooldown->start();
	_queue.push(cooldown);
	Log::debug("Triggered the cooldown of type %i (expires in %lims, started at %li)",
			std::enum_value(type), cooldown->duration(), cooldown->startMillis());
	return CooldownTriggerState::SUCCESS;
}

CooldownPtr CooldownMgr::cooldown(Type type) const {
	core::ScopedReadLock lock(_lock);
	auto i = _cooldowns.find(type);
	if (i == _cooldowns.end()) {
		return CooldownPtr();
	}
	return i->second;
}

unsigned long CooldownMgr::defaultDuration(Type type) const {
	return _cooldownProvider->duration(type);
}

bool CooldownMgr::resetCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->reset();
	return true;
}

bool CooldownMgr::cancelCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c) {
		return false;
	}
	c->cancel();
	return true;
}

bool CooldownMgr::isCooldown(Type type) {
	const CooldownPtr& c = cooldown(type);
	if (!c || !c->running()) {
		Log::trace("Cooldown of type %i is not running", std::enum_value(type));
		return false;
	}
	Log::debug("Cooldown of type %i is running and has a runtime of %lims",
			std::enum_value(type), c->duration());
	return true;
}

void CooldownMgr::update() {
	for (;;) {
		_lock.lockRead();
		if (_queue.empty()) {
			_lock.unlockRead();
			break;
		}
		CooldownPtr cooldown = _queue.top();
		_lock.unlockRead();
		if (cooldown->running()) {
			break;
		}

		_lock.lockWrite();
		_queue.pop();
		_lock.unlockWrite();
		Log::debug("Cooldown of type %i has just expired at %li",
				std::enum_value(cooldown->type()), _timeProvider->tickMillis());
		cooldown->expire();
	}
}
}
