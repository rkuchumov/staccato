#include <atomic>

#include "utils.h"
#include "deque.h"

task_deque::task_deque(size_t log_size)
{
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

	array_t *a = load_relaxed(array);

	if (b - t > a->size_mask) {
		resize();
		a = load_relaxed(array);
	}

	store_relaxed(a->buffer[b & a->size_mask], new_task);
	atomic_fence_release();

	store_relaxed(bottom, b + 1);
}

task *task_deque::take()
{
	size_t b = load_relaxed(bottom) - 1;
	array_t *a = load_relaxed(array);
	store_relaxed(bottom, b);
	atomic_fence_seq_cst();

	size_t t = load_relaxed(top);

	if (t > b) {
		store_relaxed(bottom, b + 1);
		return NULL;
	}

	task *r = load_relaxed(a->buffer[b & a->size_mask]);
	if (t == b) {
		if (!cas_strong(top, t, t + 1))
			r = NULL;
		store_relaxed(bottom, b + 1);
	}

	return r;
}

task *task_deque::steal()
{
	size_t t = load_acquire(top);
	atomic_fence_seq_cst();
	size_t b = load_acquire(bottom);

	if (t >= b)
		return NULL;

	array_t *a = load_consume(array);

	task *r = load_relaxed(a->buffer[t & a->size_mask]);
	if (!cas_weak(top, t, t + 1))
		return NULL; 

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
}
