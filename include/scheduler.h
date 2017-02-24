#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>

#include "constants.h"
#include "task.h"
#include "deque.h"
// #include "statistics.h"
#include "worker.h"

namespace staccato
{

class scheduler
{
public:
	static void initialize(size_t nthreads = 0, size_t deque_log_size = 7);
	static void terminate();

	static void spawn_and_wait(task *t);
	// static void sync();

	task *root;

// private:
	friend class task;
	friend class internal::worker;

	scheduler();
	~scheduler();

	static size_t workers_count;
	static internal::worker **workers;

	static internal::worker *get_victim(internal::worker *thief);
	static std::atomic_bool m_is_active;
	static bool is_active();

};

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

