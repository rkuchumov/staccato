#include <atomic>

#include "utils.h"
#include "deque.h"

task_deque::task_deque(size_t log_size)
{
	top = 1;
	bottom = 1;

	array_t *a = new array_t;
	a->size = 1 << log_size;
	a->buffer = new atomic_task[a->size];

	array.store(a);
}

void task_deque::put(task *new_task)
{
	size_t b = bottom.load();
	size_t t = top.load();

	array_t *a = array.load();

	if (b - t > a->size - 1) {
		resize();
		a = array.load();
	}

	a->buffer[b % a->size].store(new_task);
	bottom.store(b + 1);
}

task *task_deque::take()
{
	size_t b = bottom.load() - 1;
	array_t *a = array.load();
	bottom.store(b);
	size_t t = top.load();

	if (t > b) {
		bottom.store(b + 1);
		return NULL;
	}

	task *r = a->buffer[b % a->size].load();
	if (t == b) {
		if (!top.compare_exchange_strong(t, t + 1))
			r = NULL;
		bottom.store(b + 1);
	}

	return r;
}

task *task_deque::steal()
{
	size_t t = top.load();
	size_t b = bottom.load();

	if (t >= b)
		return NULL;

	array_t *a = array.load();

	task *r = a->buffer[t % a->size].load();
	if (!top.compare_exchange_weak(t, t + 1))
		return NULL; 

	return r;
}

void task_deque::resize()
{
	array_t *old = array.load();

	array_t *a = new array_t;
	a->size = 2 * old->size;
	a->buffer = new atomic_task[a->size];

	for (size_t i = top.load(); i < bottom.load(); i++) {
		task *item = old->buffer[i % old->size].load();
		a->buffer[i % a->size].store(item);
	}

	array.store(a);

	delete []old->buffer;
	delete old;
}
