#ifndef DEQUE_H
#define DEQUE_H

#include "utils.h"
#include "task.h"

class task_deque
{
public:
    task_deque(size_t log_size = 8);

    void put(task *t);
    task *take();
    task *steal();

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

	void resize();
};

#endif /* end of include guard: DEQUE_H */
