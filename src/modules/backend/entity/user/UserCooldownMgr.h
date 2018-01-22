/**
 * @file
 */

#pragma once

#include "cooldown/CooldownMgr.h"
#include "persistence/ForwardDecl.h"
#include "persistence/ISavable.h"
#include "backend/entity/EntityId.h"
#include "network/ServerMessageSender.h"
#include "CooldownModel.h"
#include <vector>

namespace backend {

class User;

class UserCooldownMgr : public cooldown::CooldownMgr, public persistence::ISavable {
private:
	using Super = cooldown::CooldownMgr;
	persistence::DBHandlerPtr _dbHandler;
	persistence::PersistenceMgrPtr _persistenceMgr;
	User* _user;
	mutable flatbuffers::FlatBufferBuilder _cooldownFBB;
	std::vector<db::CooldownModel> _dirtyModels;
public:
	UserCooldownMgr(User* user,
			const core::TimeProviderPtr& timeProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler,
			const persistence::PersistenceMgrPtr& persistenceMgr);

	void init() override;
	void shutdown() override;

	cooldown::CooldownTriggerState triggerCooldown(cooldown::Type type, cooldown::CooldownCallback callback = cooldown::CooldownCallback()) override;
	void sendCooldown(cooldown::Type type, bool started) const;

	bool getDirtyModels(Models& models) override;
};

}
