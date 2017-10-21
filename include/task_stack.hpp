#ifndef TASK_STACK_HPP_0BCHAEST
#define TASK_STACK_HPP_0BCHAEST

#include <cstdlib>
#include <cstdint>
#include <cstring>

namespace staccato
{
namespace internal
{

class task_stack
{
public:
	task_stack(size_t log_size, size_t elem_size)
	: elem_size(elem_size)
	, size((1 << log_size) * elem_size)
	, offset(0)
	{
		memory = new uint8_t[size];
	}

	virtual ~task_stack() {
		delete memory;
	}

	inline uint8_t *allocate() {
		if (offset >= size)
			grow();

		auto t = memory + offset;
		offset += elem_size;
		return t;
	}

	inline void deallocate() {
		offset -= elem_size;
	}

	const size_t elem_size;

private:
	inline void grow() {
		size <<= 1;
		memory = static_cast<uint8_t *> (std::realloc(memory, size));
	}

	uint8_t *memory;
	size_t size;
	size_t offset;
};

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: TASK_STACK_HPP_0BCHAEST */
