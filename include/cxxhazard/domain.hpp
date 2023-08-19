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
		_resource(), _reclaim(max) {}

	virtual ~domain(void) noexcept = default;

public:
	inline hazard_ptr make_hazard(void)
	{
		return hazard_ptr(_resource.acquire());
	}

	template <typename T>
	void retire(T *ptr)
	{
		retire(ptr, [ptr](){ delete ptr; });
	}

	template <typename T, typename Func>
	void retire(T *ptr, Func &&deleter)
	{
		if (_reclaim.emplace_back(ptr, std::forward<Func>(deleter)) >= _reclaim._max) {
			std::unordered_set<void *> is_hazard;

			for (auto p = _resource._head.load(); p != nullptr; p = p->_next)
				is_hazard.insert(p->_ptr.load());

			_reclaim.reclaim([this, &is_hazard](void *ptr){
				if (is_hazard.count(ptr) != 0)
					return true;
				return false;
			});
		}
	}

private:
	resource_pool _resource;
	reclaim_pool _reclaim;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_DOMAIN_HPP__
