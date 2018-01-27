/**
 * @file
 */

#pragma once

#include "core/TimeProvider.h"
#include "CooldownType.h"
#include "CooldownTriggerState.h"

#include <memory>
#include <functional>

namespace cooldown {

/**
 * @defgroup Cooldowns
 * @{
 */

/**
 * @brief Parameter for @c CooldownCallback
 */
enum class CallbackType {
	Started, Expired, Canceled
};

/**
 * @brief Callback that can be given to a 'new' cooldown trigger. It's called whenever the cooldown changes its state.
 * @note The @c CallbackType is given as parameter
 */
using CooldownCallback = std::function<void(CallbackType)>;

/**
 * @brief A cooldown is defined by a type, duration and a starting point.
 */
class Cooldown {
private:
	Type _type;
	unsigned long _durationMillis;
	unsigned long _startMillis;
	unsigned long _expireMillis;
	core::TimeProviderPtr _timeProvider;
	CooldownCallback _callback;

public:
	Cooldown(Type type, unsigned long durationMillis, const CooldownCallback& callback,
			const core::TimeProviderPtr& timeProvider, unsigned long startMillis = 0lu,
			unsigned long expireMillis = 0lu);

	void start();

	void reset();

	void expire();

	void cancel();

	unsigned long durationMillis() const;

	bool started() const;

	bool running() const;

	unsigned long duration() const;

	unsigned long startMillis() const;

	Type type() const;

	bool operator<(const Cooldown& rhs) const;
};

typedef std::shared_ptr<Cooldown> CooldownPtr;

/**
 * @}
 */

}

namespace std {
template<> struct hash<cooldown::Cooldown> {
	inline size_t operator()(const cooldown::Cooldown &c) const {
		return std::hash<size_t>()(static_cast<size_t>(c.type()));
	}
};
}
