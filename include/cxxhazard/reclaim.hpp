#ifndef __CXXHAZARD_RECLAIM_HPP__
#define __CXXHAZARD_RECLAIM_HPP__

#include <atomic>
#include <functional>
#include <cxxhazard/fwd.hpp>

namespace cxxhazard {

class reclaim_pool {
	friend domain;

private:
	struct node {
		node *_next;
		void *_ptr;
		std::function<void(void)> _deleter;
	};

public:
	explicit reclaim_pool(unsigned int max) :
		_head(nullptr), _cnt(0), _max(max) {}

	~reclaim_pool(void) noexcept
	{
		for (auto p = _head.load(); p != nullptr; ) {
			auto copy = p;
			p = p->_next;

			copy->_deleter();
			delete copy;
		}
	}

public:
	template <typename T, typename Func>
	int emplace(T *ptr, Func &&deleter)
	{
		node *new_node = new node;

		new_node->_ptr = ptr;
		new_node->_deleter = std::forward<Func>(deleter);
		new_node->_next = _head.load();

		while (!_head.compare_exchange_weak(new_node->_next, new_node))
			;

		return _cnt.fetch_add(1);
	}

	template <typename Func>
	void reclaim(Func &&filter)
	{
		_cnt.store(0);

		node *list = _head.exchange(nullptr);
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
			new_tail->_next = _head.load();
			
			while (!_head.compare_exchange_weak(new_tail->_next, new_head))
				;

			_cnt.fetch_add(new_size);
		}
	}

private:
	std::atomic<node *> _head;
	std::atomic<unsigned int> _cnt;
	unsigned int _max;
};

} // namespace cxxhazard

#endif // __CXXHAZARD_RECLAIM_HPP__
