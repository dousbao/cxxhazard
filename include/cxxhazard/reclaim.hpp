#ifndef __CXXHAZARD_RECLAIM_HPP__
#define __CXXHAZARD_RECLAIM_HPP__

#include <atomic>
#include <functional>
#include <cxxhazard/fwd.hpp>

namespace cxxhazard {

class reclaim_pool {
	friend enable_hazard_from_this;

private:
	struct node {
		node *_next;
		void *_ptr;
		std::function<void(void)> _deleter;
	};

public:
	reclaim_pool(void) :
		_head(nullptr), _cnt(0) {}

	~reclaim_pool(void) noexcept
	{
		for (auto p = _head.load(std::memory_order_acquire); p != nullptr; ) {
			auto copy = p;
			p = p->_next;

			copy->_deleter();
			delete copy;
		}
	}

public:
	template <typename T, typename Func>
	std::size_t emplace(T *ptr, Func &&deleter)
	{
		node *new_node = new node;

		new_node->_ptr = ptr;
		new_node->_deleter = std::forward<Func>(deleter);
		new_node->_next = _head.load(std::memory_order_acquire);

		while (!_head.compare_exchange_weak(new_node->_next, new_node,
			std::memory_order_release, std::memory_order_relaxed))
			;

		return _cnt.fetch_add(1, std::memory_order_relaxed);
	}

	template <typename Func>
	void reclaim(Func &&filter)
	{
		_cnt.store(0, std::memory_order_relaxed);

		node *list = _head.exchange(nullptr, std::memory_order_acquire);
		node *new_head = nullptr, *new_tail = nullptr;
		int new_size = 0;

		for (auto p = list; p != nullptr; ) {
			auto next = p->_next;

			if (filter(p->_ptr) == false) {
				p->_deleter();
				delete p;
			} else {
				p->_next = new_head;
				new_head = p;

				if (new_tail == nullptr)
					new_tail = p;

				++new_size;
			}

			p = next;
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
