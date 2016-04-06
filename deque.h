#ifndef DEQUE_H
#define DEQUE_H

#include "task.h"

class task_deque
{
public:
    task_deque(size_t log_size = 8);

    void put(task *t);
    task *take();
    task *steal();

private:
    std::atomic_size_t top;
    std::atomic_size_t bottom;

	typedef std::atomic<task *> atomic_task;

	typedef struct {
		size_t size;
		atomic_task *buffer;
	} array_t;

	std::atomic<array_t *> array;

	void resize();
};

#endif /* end of include guard: DEQUE_H */
