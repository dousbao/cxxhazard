#ifndef __CXXHAZARD_PTR_HPP__
#define __CXXHAZARD_PTR_HPP__

#include <cassert>
#include <cxxhazard/fwd.hpp>
#include <cxxhazard/resource.hpp>

namespace cxxhazard {

class hazard_ptr {
	friend domain;

public:
	explicit hazard_ptr(resource *res) : _res(res) {}

	hazard_ptr(const hazard_ptr &) = delete;
	hazard_ptr(hazard_ptr &&rhs) : _res(nullptr)
	{
		std::swap(_res, rhs._res);
	}

	~hazard_ptr(void) noexcept
	{
		if (_res)
			_res->unlock();
	}

	hazard_ptr &operator=(const hazard_ptr &) = delete;
	hazard_ptr &operator=(hazard_ptr &&rhs) = delete;

public:
	template <typename T>
	void protect(const std::atomic<T *> &atomic_ptr)
	{
		assert(_res != nullptr);

		while (true) {
			void *copy = atomic_ptr.load(std::memory_order_acquire);

			_res->_ptr.store(copy, std::memory_order_relaxed);

			if (copy == atomic_ptr.load(std::memory_order_acquire))
				break;
		}
	}
	
	void unprotect(void)
	{
		assert(_res != nullptr);

		_res->_ptr.store(nullptr, std::memory_order_relaxed);
	}

private:
	resource *_res;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_PTR_HPP__
