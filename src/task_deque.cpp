#include "utils.hpp"
#include "task_deque.hpp"
#include "task.hpp"
// #include "statistics.h"

namespace staccato
{
namespace internal
{

task_deque::task_deque(size_t log_size)
{
	ASSERT(log_size > 0 && log_size < 32, "Incorrect deque size.");

	m_top = 1;
	m_bottom = 1;

	array_t *a = new array_t;
	a->size_mask = (1 << log_size) - 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	store_relaxed(m_array, a);
}

task_deque::~task_deque()
{
	array_t *a = load_relaxed(m_array);

	delete []a->buffer;
	delete a;
}

void task_deque::put(task *new_task)
{
	size_t b = load_relaxed(m_bottom);
	size_t t = load_acquire(m_top);

	array_t *a = load_relaxed(m_array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(m_array);
	}

	store_relaxed(a->buffer[b & a->size_mask], new_task);

	atomic_fence_release();

	store_relaxed(m_bottom, b + 1);
}

task *task_deque::take()
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

	task *r = load_relaxed(a->buffer[b & a->size_mask]);

	if (t == b) {
		// Check if they are not stollen
		if (!cas_strong(m_top, t, t + 1)) {
			m_bottom = b + 1;
			return nullptr;
		}

		m_bottom = b + 1;
	}

	return r;
}

task *task_deque::steal()
{
	size_t t = load_acquire(m_top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(m_bottom);

	if (t >= b) { 
		return nullptr;
	}// Deque is empty

	array_t *a = load_consume(m_array);

	task *r = load_relaxed(a->buffer[t & a->size_mask]);

	// Victim doesn't have required amount of tasks
	if (!cas_weak(m_top, t, t + 1))
		return nullptr;

	return r;
}

void task_deque::resize()
{
	array_t *old = load_relaxed(m_array);

	array_t *a = new array_t;
	a->size_mask = (old->size_mask << 1) | 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	size_t t = load_relaxed(m_top);
	size_t b = load_relaxed(m_bottom);
	for (size_t i = t; i < b; i++) {
		task *item = load_relaxed(old->buffer[i & old->size_mask]);
		store_relaxed(a->buffer[i & a->size_mask], item);
	}

	store_release(m_array, a);

	delete []old->buffer;
	delete old;
}

} // namespace internal
} // namespace stacccato
