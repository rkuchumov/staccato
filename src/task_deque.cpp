#include "utils.hpp"
#include "task_deque.hpp"

namespace staccato
{
namespace internal
{

task_deque::task_deque(size_t log_size, size_t elem_size)
{
	ASSERT(log_size > 0 && log_size < 32, "Incorrect deque size.");

	m_top = 1;
	m_bottom = 1;

	array_t *a = new array_t(log_size, elem_size);

	store_relaxed(m_array, a);
}

task_deque::~task_deque()
{
	array_t *a = load_relaxed(m_array);

	delete []a->buffer;
	delete a;
}

uint8_t *task_deque::put_allocate()
{
	size_t b = load_relaxed(m_bottom);
	size_t t = load_acquire(m_top);

	array_t *a = load_relaxed(m_array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(m_array);
	}

	return a->at(b);
}

void task_deque::put_commit()
{
	size_t b = load_relaxed(m_bottom);

	atomic_fence_release();

	store_relaxed(m_bottom, b + 1);
}

uint8_t *task_deque::take(uint8_t *to)
{
	size_t b = dec_relaxed(m_bottom) - 1;
	array_t *a = load_relaxed(m_array);
	atomic_fence_seq_cst();
	size_t t = load_relaxed(m_top);

	// Deque was empty, restoring to empty state
	if (t > b) {
		store_relaxed(m_bottom, b + 1);
		return nullptr;
	}

	a->take(b, to);

	if (t == b) {
		// Check if they are not stollen
		if (!cas_strong(m_top, t, t + 1)) {
			m_bottom = b + 1;
			return nullptr;
		}

		m_bottom = b + 1;
	}

	return to;
}

uint8_t *task_deque::steal(uint8_t *to)
{
	size_t t = load_acquire(m_top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(m_bottom);

	if (t >= b) // Deque is empty
		return nullptr;

	array_t *a = load_consume(m_array);

	a->take(t, to);

	// Victim doesn't have required amount of tasks
	if (!cas_weak(m_top, t, t + 1))
		return nullptr;

	return to;
}

void task_deque::resize()
{
	array_t *old = load_relaxed(m_array);

	array_t *a = new array_t(old->log_size + 1, old->elem_size);

	size_t t = load_relaxed(m_top);
	size_t b = load_relaxed(m_bottom);
	for (size_t i = t; i < b; i++)
		a->put(i, old->at(i));

	store_release(m_array, a);

	delete []old->buffer;
	delete old;
}

} // namespace internal
} // namespace stacccato
