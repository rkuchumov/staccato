#ifndef DEQUE_H
#define DEQUE_H

#include "utils.h"
#include "task.h"

class task_deque
{
public:
	task_deque();

	void put(task *t);
	task *take();
	task *steal(task_deque *thief);

	static size_t deque_log_size;
	static size_t tasks_per_steal;

private:
	typedef std::atomic<task *> atomic_task;
	typedef struct {
		size_t size_mask; // = (2^log_size) - 1
		atomic_task *buffer;
	} array_t;

	cacheline padding_0;
	std::atomic_size_t top;

	cacheline padding_1;
	std::atomic_size_t bottom;

	cacheline padding_2;
	std::atomic<array_t *> array;
	cacheline padding_3;

	size_t load_from(array_t *buffer, size_t top, size_t s);

	void resize();
};

#endif /* end of include guard: DEQUE_H */
