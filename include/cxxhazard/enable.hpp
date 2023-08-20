#ifndef __CXXHAZARD_ENABLE_HPP__
#define __CXXHAZARD_ENABLE_HPP__

#include <unordered_set>
#include <cxxhazard/resource.hpp>
#include <cxxhazard/ptr.hpp>
#include <cxxhazard/reclaim.hpp>

namespace cxxhazard {

class enable_hazard_from_this {
public:
	explicit enable_hazard_from_this(unsigned int reclaim_level = 1000) : 
		_resource(new resource_pool), _reclaim(new reclaim_pool), _reclaim_level(reclaim_level) {}

	enable_hazard_from_this(const enable_hazard_from_this &) = delete;

	enable_hazard_from_this(enable_hazard_from_this &&rhs) :
		_resource(rhs._resource), _reclaim(rhs._reclaim), _reclaim_level(rhs._reclaim_level)
	{
		rhs._resource = nullptr;
		rhs._reclaim = nullptr;
	}

	virtual ~enable_hazard_from_this(void) noexcept
	{
		delete _resource;
		delete _reclaim;
	}

	enable_hazard_from_this &operator=(const enable_hazard_from_this &rhs) = delete;
	enable_hazard_from_this &operator=(enable_hazard_from_this &&rhs) = delete;

public:
	inline hazard_ptr make_hazard(void)
	{
		return hazard_ptr(_resource->acquire());
	}

	template <typename T>
	void retire(T *ptr)
	{
		retire(ptr, [ptr](){ delete ptr; });
	}

	template <typename T, typename Func>
	void retire(T *ptr, Func &&deleter)
	{
		if (_reclaim->emplace(ptr, std::forward<Func>(deleter)) >= _reclaim_level) {
			std::unordered_set<void *> is_hazard;

			for (auto p = _resource->_head.load(std::memory_order_acquire); p != nullptr; p = p->_next) {
				if (p->_protecting.load(std::memory_order_acquire))
					is_hazard.insert(p->_ptr.load(std::memory_order_relaxed));
			}

			_reclaim->reclaim([&is_hazard](void *ptr){
				if (is_hazard.count(ptr) != 0)
					return true;
				return false;
			});
		}
	}

private:
	resource_pool *_resource;
	reclaim_pool *_reclaim;
	std::size_t _reclaim_level;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_ENABLE_HPP__
