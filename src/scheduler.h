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

	static size_t deque_log_size;
	static size_t tasks_per_steal;

private:
	friend class task;
	friend class statistics;

	scheduler() {};
	~scheduler() {};

	static void check_paramters();

	static void initialize_worker(size_t id);
	static std::thread **workers;

	cacheline padding_0;

	static void spawn(task *t);
	static void task_loop(task *parent);

	static thread_local task_deque* my_pool;

	static std::atomic_bool is_active;
	static task_deque *pool;
	static task *steal_task();
	static size_t workers_count;

#ifndef NDEBUG
	static thread_local size_t my_id;
#endif
};

#endif /* end of include guard: SCHEDULER_H */

