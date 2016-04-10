#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstdlib>
#include <thread>

#include "task.h"
#include "deque.h"
#include "statistics.h"

class scheduler
{
public:
	static void initialize(size_t nthreads = 0);
	static void terminate();

private:
	friend class task;

	scheduler() {};
	~scheduler() {};

	static void initialize_worker(size_t id);
	static std::thread **workers;

	static void spawn(task *t);
	static void task_loop(task *parent);

	static bool is_active;
	static task_deque *pool;
	static task *steal_task();
	static size_t workers_count;

	static thread_local task_deque* my_pool;
};

#endif /* end of include guard: SCHEDULER_H */

