#ifndef __CXXHAZARD_DOMAIN_HPP__
#define __CXXHAZARD_DOMAIN_HPP__

#include <unordered_set>
#include <cxxhazard/resource.hpp>
#include <cxxhazard/ptr.hpp>
#include <cxxhazard/reclaim.hpp>

namespace cxxhazard {

class domain {
public:
	explicit domain(unsigned int max = 1000) : 
		_resource(new resource_pool), _reclaim(new reclaim_pool(max)) {}

	domain(const domain &) = delete;

	domain(domain &&rhs) :
		_resource(rhs._resource), _reclaim(rhs._reclaim)
	{
		rhs._resource = nullptr;
		rhs._reclaim = nullptr;
	}

	virtual ~domain(void) noexcept
	{
		delete _resource;
		delete _reclaim;
	}

	domain &operator=(const domain &rhs) = delete;
	domain &operator=(domain &&rhs) = delete;

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
		if (_reclaim->emplace(ptr, std::forward<Func>(deleter)) >= _reclaim->_max) {
			std::unordered_set<void *> is_hazard;

			for (auto p = _resource->_head.load(std::memory_order_acquire); p != nullptr; p = p->_next)
				is_hazard.insert(p->_ptr.load(std::memory_order_relaxed));

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
};

} // namespace cxxhazard

#endif // __CXXHAZARD_DOMAIN_HPP__
