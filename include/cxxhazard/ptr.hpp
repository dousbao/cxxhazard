#ifndef __CXXHAZARD_PTR_HPP__
#define __CXXHAZARD_PTR_HPP__

#include <cassert>
#include <cxxhazard/fwd.hpp>
#include <cxxhazard/resource.hpp>

namespace cxxhazard {

class hazard_ptr {
	friend enable_hazard_from_this;

public:
	explicit hazard_ptr(resource_pool::unique_resource *res) : _holder(res) {}

	hazard_ptr(const hazard_ptr &) = delete;
	hazard_ptr(hazard_ptr &&rhs) : _holder(nullptr)
	{
		std::swap(_holder, rhs._holder);
	}

	~hazard_ptr(void) noexcept
	{
		if (_holder) {
			unprotect();
			delete _holder;
		}
	}

	hazard_ptr &operator=(const hazard_ptr &) = delete;
	hazard_ptr &operator=(hazard_ptr &&rhs) = delete;

public:
	template <typename T>
	T *protect(const std::atomic<T *> &src)
	{
		assert(_holder != nullptr);

		while (true) {
			void *copy = src.load(std::memory_order_relaxed);

			_holder->get()->_ptr.store(copy, std::memory_order_relaxed);

			if (copy == src.load(std::memory_order_relaxed)) {
				return static_cast<T *>(copy);
			}
		}
	}
	
	void unprotect(void)
	{
		assert(_holder != nullptr);

		_holder->get()->_ptr.store(nullptr, std::memory_order_relaxed);
	}

private:
	resource_pool::unique_resource *_holder;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_PTR_HPP__
