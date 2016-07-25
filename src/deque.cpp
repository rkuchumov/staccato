#include <atomic>

#include "utils.h"
#include "deque.h"
#include "statistics.h"

namespace staccato
{
namespace internal
{

// TODO: remove it, use scheduler::log_szie;
size_t task_deque::deque_log_size = 6;
size_t task_deque::tasks_per_steal = 1;

task_deque::task_deque()
{
	ASSERT(deque_log_size > 0 && deque_log_size < 32, "Incorrect deque size.");
	ASSERT(tasks_per_steal > 0, "Incorrect number of tasks per steal.");

	stamped_size_t tt;
	tt.stamp = 0;
	tt.value = 1;

	top = tt;
	bottom = 1;

	array_t *a = new array_t;
	a->size_mask = (1 << deque_log_size) - 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	store_relaxed(array, a);
}

void task_deque::put(task *new_task)
{
	size_t b = load_relaxed(bottom);
	size_t t = load_acquire(top).value;

	array_t *a = load_relaxed(array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(array);
	}

	store_relaxed(a->buffer[b & a->size_mask], new_task);

	atomic_fence_release();

#if STACCATO_DEBUG
	ASSERT(new_task->get_state() == task::spawning,
		"Incorrect task state: " << new_task->get_state_str());
	new_task->set_state(task::ready);
#endif // STACCATO_DEBUG

	store_relaxed(bottom, b + 1);

	COUNT(put);
}

size_t task_deque::load_from(array_t *other, size_t other_top, size_t size)
{
	size_t b = load_relaxed(bottom);
	size_t t = load_acquire(top).value;

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
	stamped_size_t t = load_relaxed(top);

	// Deque was empty, restoring to empty state
	if (t.value > b) {
		store_relaxed(bottom, b + 1);
		return nullptr;
	}

	task *r = load_relaxed(a->buffer[b & a->size_mask]);

	if (t.value + (tasks_per_steal - 1) >= b) {
		// Deque had less or equal number of tasks than steal amount
		
		stamped_size_t tt;
		tt.value = t.value;
		tt.stamp = t.stamp + 1;

		// Check if they are not stollen
		if (!cas_strong(top, t, tt)) {
			bottom = b + 1;
			COUNT(take_failed);
			return nullptr;
		}
	}

#if STACCATO_DEBUG
	ASSERT(r != nullptr, "Taken NULL task from non-empty deque");
	ASSERT(r->get_state() == task::ready,
			"Incorrect task state: " << r->get_state_str());
	r->set_state(task::taken);
#endif // STACCATO_DEBUG

	COUNT(take);
	return r;
}

task *task_deque::steal(task_deque *thief)
{
	ASSERT(this != thief, "Stealing from own deque");

	stamped_size_t t = load_acquire(top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(bottom);

	if (t.value >= b) // Deque is empty
		return nullptr;

	array_t *a = load_consume(array);

	task *r = load_relaxed(a->buffer[t.value & a->size_mask]);

	if (tasks_per_steal == 1 || b - t.value < tasks_per_steal) {
		// Victim doesn't have required amount of tasks
		stamped_size_t tt;
		tt.value = t.value + 1;
		tt.stamp = t.stamp + 1;

		if (!cas_weak(top, t, tt)) {
			COUNT(single_steal_failed);
			return nullptr;
		}

#if STACCATO_DEBUG
		ASSERT(r != nullptr, "Stolen NULL task");
		ASSERT(r->get_state() == task::ready,
				"Incorrect task state: " << r->get_state_str());
		r->set_state(task::stolen);
#endif // STACCATO_DEBUG

		COUNT(single_steal);
		return r;
	}

	// Moving tasks to thief deque without updating its bottom index (in case of failed steal)
	size_t thief_b = thief->load_from(a, t.value + 1, tasks_per_steal - 1);

	// Moved tasks could be stolen or one task could be taken by owner
	stamped_size_t tt;
	tt.value = t.value + tasks_per_steal;
	tt.stamp = t.stamp + 1;
	if (!cas_weak(top, t, tt)) {
		COUNT(multiple_steal_failed);
		return nullptr; 
	}

	ASSERT(thief->bottom == thief_b, "Thief bottom index has changed");

	// Steal was successfull, updating thief bottom
	store_relaxed(thief->bottom, thief_b + tasks_per_steal - 1);

#if STACCATO_DEBUG
	ASSERT(r != nullptr, "Stolen NULL task");
	ASSERT(r->get_state() == task::ready,
		"Incorrect task state: " << r->get_state_str());
	r->set_state(task::stolen);
#endif // STACCATO_DEBUG

	COUNT(multiple_steal);
	return r;
}

void task_deque::resize()
{
	array_t *old = load_relaxed(array);

	array_t *a = new array_t;
	a->size_mask = (old->size_mask << 1) | 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	size_t t = load_relaxed(top).value;
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
size_t task_deque::get_top()
{
	return load_consume(top).value;
}
#endif // STACCATO_SAMPLE_DEQUES_SIZES

} // namespace internal
} // namespace stacccato
