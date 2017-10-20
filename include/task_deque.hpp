#ifndef STACCATO_DEQUE_H
#define STACCATO_DEQUE_H

#include <atomic>
#include <cstddef>
#include <cstring>

#include "constants.hpp"

namespace staccato
{

namespace internal
{

class task_deque
{
public:
	task_deque(size_t log_size, size_t elem_size);
	~task_deque();

	uint8_t *put_allocate();
	void put_commit();

	uint8_t *take(uint8_t *to);
	uint8_t *steal(uint8_t *to);

private:
	class array_t {
	public:
		array_t(size_t log_size, size_t elem_size)
			: log_size(log_size)
			, size_mask((1 << log_size) - 1)
			, elem_size(elem_size)
		{
			buffer = new uint8_t[elem_size * (size_mask + 1)];
		}

		const size_t log_size; // = (2^log_size) - 1
		const size_t size_mask; // = (2^log_size) - 1
		const size_t elem_size;
		uint8_t *buffer;

		inline uint8_t *at(size_t i) {
			return buffer + (i & size_mask) * elem_size;
		}

		inline void put(size_t i, uint8_t *from) {
			std::memcpy(at(i), from, elem_size);
		}

		inline void take(size_t i, uint8_t *to) {
			std::memcpy(to, at(i), elem_size);
		}
	};

	STACCATO_ALIGN std::atomic_size_t m_top;

	STACCATO_ALIGN std::atomic_size_t m_bottom;

	STACCATO_ALIGN std::atomic<array_t *> m_array;

	void resize();
};

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
