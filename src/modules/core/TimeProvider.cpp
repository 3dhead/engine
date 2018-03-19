/**
 * @file
 */

#include "TimeProvider.h"
#include "core/Assert.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <SDL.h>
#include <sys/time.h>

namespace core {

TimeProvider::TimeProvider() :
		_tickMillis(uint64_t(0)) {
}

uint64_t TimeProvider::systemMillis() const {
#if 0
	auto unix_timestamp = std::chrono::seconds(std::time(NULL));
	const uint64_t millis = unix_timestamp.count() * 1000UL;
	timeval curTime;
	gettimeofday(&curTime, nullptr);
	const uint64_t timestamp = millis + curTime.tv_usec / 1000L;
#else
	const uint64_t timestamp = SDL_GetPerformanceCounter() / 1000000UL;
#endif
#ifdef DEBUG
	static uint64_t last = 0;
#endif
#ifdef DEBUG
	core_assert_msg(timestamp >= last,
			"timestamp:%" PRIu64 ", last: %" PRIu64,
			timestamp, last);
	last = timestamp;
#endif
	return timestamp;
}

uint64_t TimeProvider::systemNanos() {
	return SDL_GetPerformanceCounter();
}

std::string TimeProvider::toString(unsigned long millis, const char *format) {
	time_t t(millis / 1000UL);
	tm tm = *gmtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, format);
	return ss.str();
}

}
