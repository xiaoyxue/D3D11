#pragma once

#include <thread>
#include <atomic>


class Spinlock {
protected:
	std::atomic<bool> latch_;

public:
	Spinlock() : Spinlock(false) {
	}

	explicit Spinlock(bool flag) {
		latch_.store(flag);
	}

	explicit Spinlock(int flag) : Spinlock(flag != 0) {}

	void lock() {
		bool unlatched = false;
		while (!latch_.compare_exchange_weak(unlatched, true,
			std::memory_order_acquire)) {
			unlatched = false;
		}
	}

	void unlock() {
		latch_.store(false, std::memory_order_release);
	}

	Spinlock(const Spinlock &o) {
		// We just ignore racing condition here...
		latch_.store(o.latch_.load());
	}

	Spinlock &operator=(const Spinlock &o) {
		// We just ignore racing condition here...
		latch_.store(o.latch_.load());
		return *this;
	}
};
