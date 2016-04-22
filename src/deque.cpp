#include <atomic>

#include "utils.h"
#include "deque.h"
#include "statistics.h"

namespace staccato
{
namespace internal
{

size_t task_deque::deque_log_size = 8;
size_t task_deque::tasks_per_steal = 1;

task_deque::task_deque()
{
	ASSERT(deque_log_size > 0 && deque_log_size < 32, "Incorrect deque size.");
	ASSERT(tasks_per_steal > 0, "Incorrect number of tasks per steal.");

	top = 1;
	bottom = 1;

	array_t *a = new array_t;
	a->size_mask = (1 << deque_log_size) - 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	store_relaxed(array, a);
}

void task_deque::put(task *new_task)
{
	size_t b = load_relaxed(bottom);
	size_t t = load_acquire(top);

	array_t *a = load_relaxed(array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(array);
	}

	store_relaxed(a->buffer[b & a->size_mask], new_task);

	atomic_fence_release();

	store_relaxed(bottom, b + 1);

	COUNT(put);
}

size_t task_deque::load_from(array_t *other, size_t other_top, size_t size)
{
	size_t b = load_relaxed(bottom);
	size_t t = load_acquire(top);

	array_t *a = load_relaxed(array);

	if (b - t + size > a->size_mask) {
		resize();
		a = load_relaxed(array);
	}

	for (size_t i = 0; i < size; i++) {
		task *tt = load_relaxed(other->buffer[(other_top + i) & other->size_mask]);
		store_relaxed(a->buffer[(b + i) & a->size_mask], tt);
	}

	atomic_fence_release();

	return b;
}

task *task_deque::take()
{
	size_t b = dec_relaxed(bottom) - 1;
	array_t *a = load_relaxed(array);
	atomic_fence_seq_cst();
	size_t t = load_relaxed(top);

	if (t > b) { // Deque was empty, restoring to empty state
		store_relaxed(bottom, b + 1);
		return nullptr;
	}

	if ((t + (tasks_per_steal - 1)) >= b) {
		// Deque had less or equal tasks tnan steal amount

		task *r = load_relaxed(a->buffer[t & a->size_mask]);

		if (!cas_strong(top, t, t + 1)) { // Check if they are not stollen
			r = nullptr;
			COUNT(take_failed);
		} else {
			COUNT(take);
		}

		ASSERT(bottom == b, "Bottom must point to the last task, as we decremented it");

		store_relaxed(bottom, b + 1);
		return r;
	}

	COUNT(take);
	return load_relaxed(a->buffer[b & a->size_mask]);
}

task *task_deque::steal(task_deque *thief)
{
	ASSERT(this != thief, "Stealing from own deque");

	size_t t = load_acquire(top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(bottom);

	if (t >= b) // Deque is empty
		return nullptr;

	array_t *a = load_consume(array);

	task *r = load_relaxed(a->buffer[t & a->size_mask]);

	if (tasks_per_steal == 1 || b - t < tasks_per_steal) {
		// Victim doesn't have required amount of tasks
		if (!cas_weak(top, t, t + 1)) {// Stealing one task
			COUNT(single_steal_failed);
			return nullptr; 
		}

		COUNT(single_steal);
		return r;
	}

	// Moving tasks to thief deque without updating its bottom index (in case of failed steal)
	size_t thief_b = thief->load_from(a, t + 1, tasks_per_steal - 1);

	// Moved tasks could be stolen or one task could be taken by owner
	if (!cas_weak(top, t, t + tasks_per_steal)) {
		COUNT(multiple_steal_failed);
		return nullptr; 
	}

	ASSERT(thief->bottom == thief_b, "Thief bottom index has changed");

	// Steal was successfull, updating thief bottom
	store_relaxed(thief->bottom, thief_b + tasks_per_steal - 1);

	COUNT(multiple_steal);
	return r;
}

void task_deque::resize()
{
	array_t *old = load_relaxed(array);

	array_t *a = new array_t;
	a->size_mask = (old->size_mask << 1) | 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	size_t t = load_relaxed(top);
	size_t b = load_relaxed(bottom);
	for (size_t i = t; i < b; i++) {
		task *item = load_relaxed(old->buffer[i & old->size_mask]);
		store_relaxed(a->buffer[i & a->size_mask], item);
	}

	store_release(array, a);

	delete []old->buffer;
	delete old;

	COUNT(resize);
}

#if STACCATO_SAMPLE_DEQUES_SIZES
ssize_t task_deque::size()
{
	size_t t = load_consume(top);
	size_t b = load_consume(bottom);

	return (ssize_t) b - t;
}
#endif // STACCATO_SAMPLE_DEQUES_SIZES

} // namespace internal
} // namespace stacccato
