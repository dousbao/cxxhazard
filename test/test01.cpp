#include <cxxhazard/hazard.hpp>
#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>

template <typename T>
class stack : private cxxhazard::enable_hazard_from_this {
public:
	using value_type = T;

	struct node {
		node *_next;
		value_type *_data;
	};

public:
	stack(void) : cxxhazard::enable_hazard_from_this(0) {}

	~stack(void) noexcept
	{
		for (auto p = _head.load(); p != nullptr; ) {
			auto copy = p;
			p = p->_next;

			delete copy->_data;
			delete copy;
		}
	}

public:
	void push(value_type data)
	{
		auto new_node = new node;

		try {
			new_node->_data = new value_type(data);
			new_node->_next = _head.load();

			while (!_head.compare_exchange_weak(new_node->_next, new_node))
				;

		} catch (...) { delete new_node; throw std::current_exception(); }
	}

	bool pop(T &dest)
	{
		auto old_head = _head.load();
		auto hazard = make_hazard();

		do {
			old_head = hazard.protect(_head);

			if (old_head == nullptr)
				return false;

		} while (!_head.compare_exchange_strong(old_head, old_head->_next));
		hazard.unprotect();

		dest = *old_head->_data;

		retire(old_head, [old_head](){
			delete old_head->_data;
			delete old_head;
		});

		return true;
	}

	bool peek(T &dest)
	{
		auto hazard = make_hazard();
		auto old_head = hazard.protect(_head);

		if (old_head) {
			dest = *old_head->_data;
			return true;
		}
		return false;
	}

private:
	std::atomic<node *> _head;
};

std::vector<std::thread> pool;
std::atomic<int> cnt = 100000;
stack<int> s;

int main(void)
{
	for (int i = 0; i < cnt; ++i)
		s.push(i);

	for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
		pool.emplace_back([](){
			int buf;
			while (s.peek(buf))
				;
		});

	for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
		pool.emplace_back([](){
			int buf;
			while (s.pop(buf)) {
				--cnt;
			}
		});

	for (auto &thread : pool)
		if (thread.joinable())
			thread.join();

	std::cout << cnt << std::endl;
	assert(cnt == 0);
}
