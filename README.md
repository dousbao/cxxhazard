# Project: cxxhazard
A complete hazard pointer implementation in modern C++ (C++17), with modular design, heavy comment and clean structure.

# Usage
Here is a simple demo with incomplete code. For a complete program, refers to this [file](test/test01.cpp)
```
template <typename T>
class stack : private cxxhazard::enable_hazard_from_this {
public:
	struct node {
		node *_next;
		value_type *_data;
	};

public:
// constructor/destructor/operator(s)...

public:
// push/size/empty/etc...

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
```

# Project Structure
| File | Purpose |
| ---------- | --------------- |
| [hazard.hpp](include/cxxhazard/hazard.hpp) | global header |
| [fwd.hpp](include/cxxhazard/fwd.hpp) | forward declarations |
| [resource.hpp](include/cxxhazard/resource.hpp) | internal hazard_ptr data storage/manager |
| [ptr.hpp](include/cxxhazard/ptr.hpp) | hazard_ptr interface |
| [reclaim.hpp](include/cxxhazard/reclaim.hpp) | retired node storage |
| [domain.hpp](include/cxxhazard/domain.hpp) | necessary data storage/base class |

# TODO
* Add more test unit, more test
* Provide extra example
* Better query data structure for resource_pool
