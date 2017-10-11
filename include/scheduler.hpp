#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>
#include <atomic>
#include <functional>

#include "constants.hpp"

namespace staccato
{

class task;

namespace internal
{
class worker;
}

class scheduler
{
public:
	static void initialize(size_t nthreads = 0, size_t deque_log_size = 7);
	static void terminate();

	static void spawn_and_wait(task *t);

	static void spawn(task *t);

	static void spawn(std::function <void()> fn);

	static void wait();

private:
	friend class task;
	friend class internal::worker;

	scheduler();
	~scheduler();

	enum state_t { 
		terminated,
		initializing,
		initialized,
		terminating,
	};
	static std::atomic<state_t> state;

	static task *root;

	static void wait_workers_fork();

	static size_t workers_count;
	static internal::worker **workers;


	static void wait_until_initilized();

	static internal::worker *get_victim(internal::worker *thief);
};

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

