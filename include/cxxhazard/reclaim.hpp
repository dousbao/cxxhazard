/**
 * @fiel reclaim.hpp
 * @brief The file provides interface to manage retired nodes.
 *
 * @author dousbao
 * @date Aug 25 2023
 */

#ifndef __CXXHAZARD_RECLAIM_HPP__
#define __CXXHAZARD_RECLAIM_HPP__

#include <atomic>
#include <functional>
#include <exception>
#include <type_traits>
#include <cxxhazard/fwd.hpp>

namespace cxxhazard {

/**
 * @class reclaim_pool
 * @brief The class serves as retired node storage/reclaimer
 */

class reclaim_pool {
	friend enable_hazard_from_this;

private:
	struct node {
		node *_next;
		void *_ptr;
		std::function<void(void)> _deleter;

		~node(void) noexcept
		{
			_deleter();
		}
	};

public:
	reclaim_pool(void) :
		_head(nullptr), _cnt(0) {}

	~reclaim_pool(void) noexcept
	{
		for (auto p = _head.load(std::memory_order_acquire); p != nullptr; ) {
			auto copy = p;
			p = p->_next;
			delete copy;
		}
	}

public:

	/**
	 * @brief Emplace a new (internal) node to pool.
	 *
	 * NOTE: meet exception when and only when copy/move assignmen operator
	 *     of deleter throw an exception.
	 *
	 * @param ptr Pointer to retire
	 * @param deleter Function to delete ptr
	 *
	 * @return Size of pool before this emplace
	 */

	template <typename T, typename Func>
	std::size_t emplace(T *ptr, Func &&deleter)
	{
		static_assert(std::is_nothrow_invocable_v<Func>);

		node *new_node = new node;

		try {
			new_node->_ptr = ptr;
			new_node->_deleter = std::forward<Func>(deleter);
			new_node->_next = _head.load(std::memory_order_acquire);
		} catch (...) { delete new_node; throw std::current_exception(); }

		while (!_head.compare_exchange_weak(new_node->_next, new_node,
			std::memory_order_release, std::memory_order_relaxed))
			;

		return _cnt.fetch_add(1, std::memory_order_relaxed);
	}

	/**
	 * @brief Reclaim the retired nodes, as long as its not hazard.
	 *
	 * The function takes a functor with signature bool(void *) to decide
	 * whether a given pointer should be reclaimed (whether it's hazard).
	 * If functor return false, ptr will be reclaimed. Otherwise, does nothing.
	 *
	 * @param filter Function to decide whether a pointer can be safely reclaimed.
	 */

	template <typename Func>
	void reclaim(Func &&filter) noexcept
	{
		_cnt.store(0, std::memory_order_relaxed);

		node *list = _head.exchange(nullptr, std::memory_order_acquire);
		node *new_head = nullptr, *new_tail = nullptr;
		int new_size = 0;

		while (list) {
			auto next = list->_next;

			if (filter(list->_ptr) == false) {
				delete list;
			} else {
				list->_next = new_head;
				new_head = list;

				if (new_tail == nullptr)
					new_tail = list;

				++new_size;
			}

			list = next;
		}

		if (new_head) {
			new_tail->_next = _head.load(std::memory_order_acquire);
			
			while (!_head.compare_exchange_weak(new_tail->_next, new_head,
				std::memory_order_release, std::memory_order_relaxed))
				;

			_cnt.fetch_add(new_size, std::memory_order_relaxed);
		}
	}

private:
	std::atomic<node *> _head;
	std::atomic<std::size_t> _cnt;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_RECLAIM_HPP__
