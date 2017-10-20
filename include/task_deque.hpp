#ifndef STACCATO_DEQUE_H
#define STACCATO_DEQUE_H

#include <atomic>
#include <cstddef>
#include <cstring>

#include "constants.hpp"
#include "buffer.hpp"

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
	STACCATO_ALIGN std::atomic_size_t m_top;

	STACCATO_ALIGN std::atomic_size_t m_bottom;

	STACCATO_ALIGN std::atomic<buffer *> m_array;

	void resize();
};

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
