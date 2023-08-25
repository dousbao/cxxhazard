/**
 * @file domain.hpp
 * @brief The file provides class to define domain of hazard pointer
 *
 * @author dousbao
 * @date Aug 25 2023
 */

#ifndef __CXXHAZARD_DOMAIN_HPP__
#define __CXXHAZARD_DOMAIN_HPP__

#include <cxxhazard/resource.hpp>
#include <cxxhazard/ptr.hpp>
#include <cxxhazard/reclaim.hpp>

namespace cxxhazard {

/**
 * @class enable_hazard_from_this
 * @brief The class defines domain of hazard pointer.
 *
 * By inherit from this class, user could restrict the domain of hazard pointer
 * to each derived class's instance. That, each instance have its own hazard pointer
 * resources (retired node list, hazard ptr list, etc). The restriction also limits the
 * visibility of hazard pointer interface.
 *
 * Only move constructor is allowed, since share/overwrite data would be dangerous for lock
 * free data structure.
 */

class enable_hazard_from_this {
public:
	explicit enable_hazard_from_this(std::size_t reclaim_level = 1000) : 
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
		if (_resource)
			delete _resource;
		if (_reclaim)
			delete _reclaim;
	}

	enable_hazard_from_this &operator=(const enable_hazard_from_this &rhs) = delete;
	enable_hazard_from_this &operator=(enable_hazard_from_this &&rhs) = delete;

public:

	/**
	 * @brief Make a instance of hazard pointer
	 *
	 * @return Hazard pointer instance belongs to current domain
	 */

	inline hazard_ptr make_hazard(void)
	{
		return hazard_ptr(_resource->make_unique_resource());
	}

	/**
	 * @brief Retire given pointer with default deleter
	 *
	 * @param ptr Pointer to be retired
	 */

	template <typename T>
	void retire(T *ptr)
	{
		retire(ptr, [ptr]() noexcept { delete ptr; });
	}

	/**
	 * @brief Retire given pointer with given deleter
	 *
	 * NOTE: It would lead to bug if using a cache here. For instance:
	 *
	 * DEF map is_hazard
	 * FOR-EACH node FROM _resource
	 *     IF node IS hazard
	 *         INSERT node TO map
	 *
	 * CALL reclaim WITH is_hazard
	 *
	 * The code above has problem, imagine:
	 * Assume:
	 *     Threads: A, B, C
	 *     Sharing nodes: N1, N2
	 *
	 * Execution Flow:
	 *     A: protect N1 -> CAS (remove N1 from sharing data structure) ->
	 *         unprotect N1 -> retire N1 -> add N1 to retire list -> build is_hazard cache
	 *     B: protect N2
	 *     C: protect N2 -> CAS N2 -> unprotect N2 -> retire N2 -> add N2 to retire list
	 *     A: reclaim -> delete N2, N1 based on is_hazard cache
	 *     B: ERROR! USING PROTECTING POINTER WHICH IS ACTUALLY DELETED, UN-DEFINED BEHAVIOR!
	 *
	 * Therefore, the code currently dynamatically check whether ptr is hazard. 
	 * TODO: The current approch is reasonable but ineffiencient when resource becomes long.
	 *     A better query data structure should be used other than pure link list.
	 *
	 * @param ptr Pointer to be retired
	 * @param deleter Customized deleter for reclaim
	 */

	template <typename T, typename Func>
	void retire(T *ptr, Func &&deleter)
	{
		if (_reclaim->emplace(ptr, std::forward<Func>(deleter)) >= _reclaim_level) {
			_reclaim->reclaim([this](const void *ptr){
				for (auto p = _resource->_head.load(std::memory_order_acquire); p != nullptr; p = p->_next)
					if (p->_ptr.load(std::memory_order_acquire) == ptr)
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

#endif // __CXXHAZARD_DOMAIN_HPP__
