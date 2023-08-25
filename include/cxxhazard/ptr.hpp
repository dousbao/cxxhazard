/**
 * @file ptr.hpp
 * @brief The files provides definition of hazard_ptr
 *
 * @author dousbao
 * @date Aug 25 2023
 */

#ifndef __CXXHAZARD_PTR_HPP__
#define __CXXHAZARD_PTR_HPP__

#include <cassert>
#include <cxxhazard/fwd.hpp>
#include <cxxhazard/resource.hpp>

namespace cxxhazard {

/**
 * @class hazard_ptr
 * @brief The class provide interface to safely protect/unprotect external pointer
 *
 * NOTE: The class is not a RAII class, which acquire/release resource during
 *     construction/destruction, but only provide interface.
 */

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
	/**
	 * @brief Mark given pointer as hazard
	 *
	 * The function takes an external atomic variable which stores a pointer.
	 * The function will keeps looping until the marked pointer is the 
	 *
	 * NOTE: The paramter must be reference to external storage, since other threads
	 * might "remove" that pointer already before it is marked as hazard.
	 *
	 * @param src Atomic reference to external storage, which stores the pointer want to mark
	 */

	template <typename T>
	T *protect(const std::atomic<T *> &src) noexcept
	{
		while (true) {
			void *copy = src.load(std::memory_order_relaxed);

			_holder->get()->_ptr.store(copy, std::memory_order_release);

			if (copy == src.load(std::memory_order_relaxed)) {
				return static_cast<T *>(copy);
			}
		}
	}

	/**
	 * @brief Clear the protecting pointer
	 */
	
	inline void unprotect(void) noexcept
	{
		_holder->get()->_ptr.store(nullptr, std::memory_order_release);
	}

private:
	resource_pool::unique_resource *_holder;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_PTR_HPP__
