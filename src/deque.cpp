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

	array.store(a);
}

void task_deque::put(task *new_task)
{
	size_t b = bottom.load(std::memory_order_relaxed);
	size_t t = top.load(std::memory_order_acquire);

	array_t *a = array.load(std::memory_order_relaxed);

	if (b - t > a->size_mask) {
		resize();
		a = array.load(std::memory_order_relaxed);
	}

	a->buffer[b & a->size_mask].store(new_task, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_release);
	bottom.store(b + 1, std::memory_order_relaxed);
}

task *task_deque::take()
{
	size_t b = bottom.load(std::memory_order_relaxed) - 1;
	array_t *a = array.load(std::memory_order_relaxed);
	bottom.store(b, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_seq_cst);
	size_t t = top.load(std::memory_order_relaxed);

	if (t > b) {
		bottom.store(b + 1, std::memory_order_relaxed);
		return NULL;
	}

	task *r = a->buffer[b & a->size_mask].load(std::memory_order_relaxed);
	if (t == b) {
		if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
			r = NULL;
		bottom.store(b + 1, std::memory_order_relaxed);
	}

	return r;
}

task *task_deque::steal()
{
	size_t t = top.load(std::memory_order_acquire);
	std::atomic_thread_fence(std::memory_order_seq_cst);
	size_t b = bottom.load(std::memory_order_acquire);

	if (t >= b)
		return NULL;

	array_t *a = array.load(std::memory_order_consume);

	task *r = a->buffer[t & a->size_mask].load(std::memory_order_relaxed);
	if (!top.compare_exchange_weak(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
		return NULL; 

	return r;
}

void task_deque::resize()
{
	array_t *old = array.load();

	array_t *a = new array_t;
	a->size_mask = (old->size_mask << 1) | 1;
	a->buffer = new atomic_task[a->size_mask + 1];

	for (size_t i = top.load(); i < bottom.load(); i++) {
		task *item = old->buffer[i & old->size_mask].load();
		a->buffer[i & a->size_mask].store(item);
	}

	array.store(a);

	delete []old->buffer;
	delete old;
}
