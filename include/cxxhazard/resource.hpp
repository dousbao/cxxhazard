#ifndef __CXXHAZARD_RESOURCE_HPP__
#define __CXXHAZARD_RESOURCE_HPP__

#include <atomic>
#include <cxxhazard/fwd.hpp>

namespace cxxhazard {

class resource {
	friend domain;
	friend resource_pool;
	friend hazard_ptr;

public:
	resource(void) : 
		_ptr(nullptr), _next(nullptr), _active(ATOMIC_FLAG_INIT) {}

	resource(const resource &) = delete;
	resource(resource &&) = delete;

	resource &operator=(const resource &) = delete;
	resource &operator=(resource &&) = delete;

public:
	inline bool try_lock(void) noexcept
	{
		return !_active.test_and_set(std::memory_order_acquire);
	}

	inline void lock(void) noexcept
	{
		while (!try_lock())
			;
	}

	inline void unlock(void) noexcept
	{
		_active.clear(std::memory_order_release);
	}

private:
	std::atomic<void *>_ptr;
	resource *_next;
	std::atomic_flag _active;
};

class resource_pool {
	friend domain;

public:
	~resource_pool(void) noexcept
	{
		for (auto p = _head.load(); p != nullptr; ) {
			auto copy = p;
			p = p->_next;
			delete copy;
		}
	}

public:
	resource *acquire(void)
	{
		auto old_head = _head.load(std::memory_order_acquire);

		for (auto p = old_head; p != nullptr; p = p->_next) {
			if (p->try_lock())
				return p;
		}

		resource *new_res = new resource;

		new_res->lock();
		new_res->_next = old_head;

		while (!_head.compare_exchange_weak(new_res->_next, new_res, 
			std::memory_order_release, std::memory_order_relaxed))
			;

		return new_res;
	}

	inline void release(resource *res) noexcept
	{
		res->unlock();
	}

private:
	std::atomic<resource *> _head;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_RESOURCE_HPP__
