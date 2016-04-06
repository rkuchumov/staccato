#include <atomic>

#include "utils.h"
#include "deque.h"

task_deque::task_deque(size_t log_size)
{
	size = 1 << log_size;
	top = 1;
	bottom = 1;

	array = new std::atomic<task *>[size];
}

void task_deque::put(task *new_task)
{
	size_t b = bottom.load();
	array[b % size].store(new_task);
	bottom.store(b + 1);
}

task *task_deque::take()
{
	size_t b = bottom.fetch_sub(1) - 1;
	size_t t = top.load();

	task *r;
	if (t <= b) {
		r = array[b % size].load();
		if (t == b) {
			if (!top.compare_exchange_strong(t, t + 1))
				r = NULL;
			bottom.store(b + 1);
		}
	} else {
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

	task *r = array[t % size].load();
	if (!top.compare_exchange_weak(t, t + 1))
		return NULL; 

	return r;
}

