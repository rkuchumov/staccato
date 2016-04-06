#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstdlib>
#include <thread>

#include "task.h"
#include "deque.h"

class scheduler
{
public:
	static void initialize(size_t nthreads);
	static void terminate();

private:
	scheduler() {};
	~scheduler() {};

	friend class task;
	static void task_loop(task *parent);
	static void spawn(task *t);
	static task *steal_task();

	static task_deque *pool;
	static std::thread **workers;

	static void initialize_worker(size_t id);

	static unsigned thread_local me;

	static bool is_active;

	static size_t workers_count;
};

#endif /* end of include guard: SCHEDULER_H */

