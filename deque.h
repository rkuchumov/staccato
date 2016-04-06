#ifndef DEQUE_H
#define DEQUE_H

#include "task.h"

class task_deque
{
public:
    task_deque(size_t log_size = 13);

    void put(task *t);
    task *take();
    task *steal();

private:
	size_t size;

    std::atomic_size_t top;
    std::atomic_size_t bottom;
	std::atomic<task *> *array;

};

#endif /* end of include guard: DEQUE_H */
