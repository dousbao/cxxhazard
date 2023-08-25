/**
 * @file resource.hpp
 * @brief This file provides interface to handle resource (internal data of each hazard pointer)
 *
 * @author dousbao
 * @date Aug 25 2023
 */

#ifndef __CXXHAZARD_RESOURCE_HPP__
#define __CXXHAZARD_RESOURCE_HPP__

#include <atomic>
#include <cxxhazard/fwd.hpp>

namespace cxxhazard {

/**
 * @class resource
 * @brief Basic building block of resource management interface.
 *
 * The class serves as storage of internal hazard pointer's information.
 * The class also use extra bit as flag to lock/unlock the resource.
 */

class resource {
	friend enable_hazard_from_this;
	friend resource_pool;
	friend hazard_ptr;

public:
	resource(void) : 
		_ptr(nullptr), _next(nullptr), _locking(ATOMIC_FLAG_INIT) {}

	resource(const resource &) = delete;
	resource(resource &&) = delete;

	resource &operator=(const resource &) = delete;
	resource &operator=(resource &&) = delete;

public:

	/**
	 * @brief Try to lock the resource's ownership without blocking.
	 *
	 * @return True when lock the resource successfully, false otherwise.
	 */

	inline bool try_lock(void) noexcept
	{
		return !_locking.test_and_set(std::memory_order_acquire);
	}

	/**
	 * @brief Lock the resource's ownership, block until success.
	 */

	inline void lock(void) noexcept
	{
		while (!try_lock())
			;
	}

	/**
	 * @brief Unlock the resource's ownership.
	 */

	inline void unlock(void) noexcept
	{
		_locking.clear(std::memory_order_release);
	}

private:
	std::atomic<void *> _ptr;			/// Used by hazard_ptr
	resource *_next;					/// Used by resource_pool
	std::atomic_flag _locking;			/// Used by this
};

/**
 * @class resource_pool
 * @brief A dynamic pool (list) of resources.
 */

class resource_pool {
	friend enable_hazard_from_this;

public:

	/**
	 * @class unique_resource
	 * @brief RAII resource ownership handler.
	 */

	class unique_resource {
	public:
		explicit unique_resource(resource_pool &pool) :
			_pool(pool), _res(pool.acquire()) {}

		unique_resource(const unique_resource &) = delete;
		unique_resource(unique_resource &&) = delete;

		~unique_resource(void) noexcept
		{
			_pool.release(_res);
		}

		unique_resource &operator=(const unique_resource &) = delete;
		unique_resource &operator=(unique_resource &&) = delete;

	public:
		
		/**
		 * @brief Get the underlying resource
		 *
		 * @return Underlying resource
		 */

		inline resource *get(void) noexcept
		{
			return _res;
		}

	private:
		resource_pool &_pool;
		resource *_res;
	};

public:
	resource_pool(void) : _head(nullptr) {}
	
	~resource_pool(void) noexcept
	{
		for (auto p = _head.load(); p != nullptr; ) {
			auto copy = p;
			p = p->_next;
			delete copy;
		}
	}

public:

	/**
	 * @brief Construct a unique_resource from current pool instance
	 *
	 * @return new unique_resource instance
	 */

	inline unique_resource *make_unique_resource(void)
	{
		return new unique_resource(*this);
	}

	/**
	 * @brief Acquire a piece of resource from current pool instance
	 *
	 * Acquire a piece of resource from this pool.
	 * If there exist an unlcoked resource, take the ownership of that and return.
	 * If there does not exist an unlocked resource, create one dynamatically,
	 * add to pool and return.
	 *
	 * @return Resource instance
	 */

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

	/**
	 * @brief Release given resource at the level of resource_pool.
	 *
	 * @param res Resource instance
	 */

	inline void release(resource *res) noexcept
	{
		res->unlock();
	}

private:
	std::atomic<resource *> _head;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_RESOURCE_HPP__
