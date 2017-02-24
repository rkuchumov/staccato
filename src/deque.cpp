#include <atomic>

#include "utils.h"
#include "deque.h"
// #include "statistics.h"

namespace staccato
{
namespace internal
{

task_deque::task_deque(size_t log_size)
{
	ASSERT(log_size > 0 && log_size < 32, "Incorrect deque size.");

	top = 1;
	bottom = 1;

	array_t *a = new array_t;
	a->size_mask = (1 << log_size) - 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	store_relaxed(array, a);
}

void task_deque::put(task *new_task)
{
	size_t b = load_relaxed(bottom);
	size_t t = load_acquire(top);

	std::cout << "put: " << b << " " << t << std::endl;

	array_t *a = load_relaxed(array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(array);
	}

	store_relaxed(a->buffer[b & a->size_mask], new_task);

	atomic_fence_release();

#if STACCATO_DEBUG
	// ASSERT(new_task->get_state() == task::spawning,
	// 	"Incorrect task state: " << new_task->get_state_str());
	// new_task->set_state(task::ready);
#endif // STACCATO_DEBUG

	store_relaxed(bottom, b + 1);

	COUNT(put);
}

task *task_deque::take()
{
	size_t b = dec_relaxed(bottom) - 1;
	array_t *a = load_relaxed(array);
	atomic_fence_seq_cst();
	size_t t = load_relaxed(top);

	std::cout << "take: " << b + 1 << " " << t << std::endl;

	// Deque was empty, restoring to empty state
	if (t > b) {
		store_relaxed(bottom, b + 1);
		return nullptr;
	}

	task *r = load_relaxed(a->buffer[b & a->size_mask]);

	if (t == b) {
		// std::cout << "one task in deque" << std::endl;
		// Check if they are not stollen
		if (!cas_strong(top, t, t + 1)) {
			bottom = b + 1;
			COUNT(take_failed);
			return nullptr;
		}

		bottom = b + 1;
	}

#if STACCATO_DEBUG
	// ASSERT(r != nullptr, "Taken NULL task from non-empty deque");
	// ASSERT(r->get_state() == task::ready,
	// 		"Incorrect task state: " << r->get_state_str());
	// r->set_state(task::taken);
#endif // STACCATO_DEBUG

	COUNT(take);
	return r;
}

task *task_deque::steal()
{
	// std::cout << "steal() is called" << std::endl;
	// ASSERT(this != thief, "Stealing from own deque");

	size_t t = load_acquire(top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(bottom);

	if (t >= b) { 
		std::cerr << "was empty\n";
		return nullptr;
	}// Deque is empty

	array_t *a = load_consume(array);

	task *r = load_relaxed(a->buffer[t & a->size_mask]);

	// Victim doesn't have required amount of tasks

	if (!cas_weak(top, t, t + 1)) {
		COUNT(single_steal_failed);
		std::cerr << "steal failed\n";
		return nullptr;
	}

	std::cerr << "steal success\n";

#if STACCATO_DEBUG
	// ASSERT(r != nullptr, "Stolen NULL task");
	// ASSERT(r->get_state() == task::ready,
	// 		"Incorrect task state: " << r->get_state_str());
	// r->set_state(task::stolen);
#endif // STACCATO_DEBUG

	COUNT(single_steal);
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
size_t task_deque::get_top()
{
	return load_consume(top).value;
}
#endif // STACCATO_SAMPLE_DEQUES_SIZES

} // namespace internal
} // namespace stacccato
