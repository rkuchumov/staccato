#ifndef BUFFER_HPP_9RO0BFAQ
#define BUFFER_HPP_9RO0BFAQ

#include <cstdlib>
#include <cstdint>
#include <cstring>

namespace staccato
{
namespace internal
{

class buffer {
public:
	buffer(size_t log_size, size_t elem_size)
	: log_size(log_size)
	, mask((1 << log_size) - 1)
	, elem_size(elem_size)
	{
		memory = new uint8_t[elem_size * (mask + 1)];
	}

	virtual ~buffer()
	{
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

#endif /* end of include guard: BUFFER_HPP_9RO0BFAQ */
