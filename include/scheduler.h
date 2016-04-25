#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>

#include "constants.h"
#include "task.h"
#include "deque.h"
#include "statistics.h"

namespace staccato
{

class scheduler
{
public:
	static void initialize(size_t nthreads = 0);
	static void terminate();

	static size_t deque_log_size;
	static size_t tasks_per_steal;

private:
	friend class task;
	friend class internal::statistics;

	scheduler();
	~scheduler();

	static void initialize_worker(size_t id);
	static std::thread **workers;

	static void spawn(task *t);
	static void task_loop(task *parent);

	static STACCATO_TLS internal::task_deque* my_pool;

	static std::atomic_bool is_active;
	static internal::task_deque *pool;
	static task *steal_task();
	static size_t workers_count;

#if STACCATO_DEBUG
	static STACCATO_TLS size_t my_id;
#endif
};

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

