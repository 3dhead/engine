#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace core {

template<class Data, class Compare = std::less<Data> >
class ConcurrentQueue {
private:
	using Collection = std::priority_queue<Data, std::vector<Data>, Compare>;
	Collection _data;
	mutable std::mutex _mutex;
	std::condition_variable _conditionVariable;
	std::atomic_bool _abort { false };
public:
	using Key = Data;

	~ConcurrentQueue() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		std::unique_lock<std::mutex> lock(_mutex);
		_conditionVariable.notify_all();
	}

	void clear() {
		std::unique_lock<std::mutex> lock(_mutex);
		_data = Collection();
	}

	void push(Data const& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_data.push(data);
		lock.unlock();
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_data.push(data);
		lock.unlock();
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _data.size();
	}

	bool pop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = std::move(_data.top());
		_data.pop();
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_data.empty()) {
			_conditionVariable.wait(lock, [this] {
				return _abort || !_data.empty();
			});
			if (_abort) {
				_abort = false;
				return false;
			}
			if (_data.empty()) {
				return false;
			}
		}

		poppedValue = std::move(_data.top());
		_data.pop();
		return true;
	}
};

}
