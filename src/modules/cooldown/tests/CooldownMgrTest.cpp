/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "cooldown/CooldownMgr.h"
#include "cooldown/CooldownDuration.h"
#include "core/Singleton.h"

namespace cooldown {

class CooldownMgrTest : public core::AbstractTest {
protected:
	core::TimeProviderPtr _timeProvider;
	CooldownMgr _mgr;
public:
	CooldownMgrTest() :
		_timeProvider(std::make_shared<core::TimeProvider>()), _mgr(_timeProvider) {
	}

	void SetUp() override {
		core::AbstractTest::SetUp();
		core::Singleton<cooldown::CooldownDuration>::getInstance().init("cooldowns.lua");
	}
};

TEST_F(CooldownMgrTest, testTriggerCooldown) {
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
}

TEST_F(CooldownMgrTest, testCancelCooldown) {
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_TRUE(_mgr.cancelCooldown(Type::LOGOUT)) << "Failed to cancel the logout cooldown";
}

TEST_F(CooldownMgrTest, testExpireCooldown) {
	_timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(_mgr.defaultDuration(Type::LOGOUT), _mgr.cooldown(Type::LOGOUT)->durationMillis());
	ASSERT_EQ(_mgr.defaultDuration(Type::LOGOUT), _mgr.cooldown(Type::LOGOUT)->duration());
	ASSERT_TRUE(_mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	_mgr.update();
	ASSERT_TRUE(_mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	_timeProvider->update(_mgr.defaultDuration(Type::LOGOUT));
	ASSERT_FALSE(_mgr.isCooldown(Type::LOGOUT));
	_mgr.update();
	ASSERT_FALSE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is still running";
	ASSERT_FALSE(_mgr.isCooldown(Type::LOGOUT));
	ASSERT_TRUE(_mgr.resetCooldown(Type::LOGOUT)) << "Failed to reset the logout cooldown";
}

TEST_F(CooldownMgrTest, testMultipleCooldown) {
	_timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::INCREASE)) << "Increase cooldown couldn't get triggered";
	ASSERT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	ASSERT_TRUE(_mgr.isCooldown(Type::INCREASE));
	_mgr.update();
	ASSERT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	ASSERT_TRUE(_mgr.isCooldown(Type::INCREASE));

	const unsigned long logoutDuration = _mgr.defaultDuration(Type::LOGOUT);
	const unsigned long increaseDuration = _mgr.defaultDuration(Type::INCREASE);

	if (logoutDuration > increaseDuration) {
		_timeProvider->update(increaseDuration);
		_mgr.update();
		ASSERT_TRUE(_mgr.isCooldown(Type::LOGOUT));
		ASSERT_FALSE(_mgr.isCooldown(Type::INCREASE));
	} else {
		_timeProvider->update(logoutDuration);
		_mgr.update();
		ASSERT_TRUE(_mgr.isCooldown(Type::INCREASE));
		ASSERT_FALSE(_mgr.isCooldown(Type::LOGOUT));
	}
}

TEST_F(CooldownMgrTest, testTriggerCooldownTwice) {
	ASSERT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::ALREADY_RUNNING, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown was triggered twice";
}

}
