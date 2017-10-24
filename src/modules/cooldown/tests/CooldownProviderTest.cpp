/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "cooldown/CooldownProvider.h"

namespace cooldown {

class CooldownDurationTest : public core::AbstractTest {
};

TEST_F(CooldownDurationTest, testLoading) {
	CooldownProvider d;
	const std::string& cooldowns = core::App::getInstance()->filesystem()->load("cooldowns.lua");
	ASSERT_TRUE(d.init(cooldowns)) << d.error();
	ASSERT_NE(DefaultDuration, d.duration(Type::LOGOUT));
}

}
