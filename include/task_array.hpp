#ifndef TASK_ARRAY_HPP_ZQH1IMCH
#define TASK_ARRAY_HPP_ZQH1IMCH

#include <cstdlib>
#include <cstdint>
#include <cstring>

namespace staccato
{
namespace internal
{

class task_array {
public:
	task_array(size_t log_size, size_t elem_size)
	: log_size(log_size)
	, mask((1 << log_size) - 1)
	, elem_size(elem_size)
	{
		memory = new uint8_t[elem_size * (mask + 1)];
	}

	virtual ~task_array() {
		delete memory;
	}

	inline uint8_t *at(size_t i) {
		return memory + (i & mask) * elem_size;
	}

	inline void put(size_t i, uint8_t *from) {
		std::memcpy(at(i), from, elem_size);
	}

	inline void take(size_t i, uint8_t *to) {
		std::memcpy(to, at(i), elem_size);
	}

	const size_t log_size;
	const size_t mask;
	const size_t elem_size;

private:
	uint8_t *memory;
};

} /* internal */ 
	
} /* staccato */ 

#endif /* end of include guard: TASK_ARRAY_HPP_ZQH1IMCH */
