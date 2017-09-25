#include "utils.hpp"
#include "scheduler.hpp"
#include "worker.hpp"
#include "task.hpp"

namespace staccato
{
using namespace internal;

worker **scheduler::workers;
size_t scheduler::workers_count(0);
std::atomic<scheduler::state_t> scheduler::state(terminated);

scheduler::scheduler()
{ }

scheduler::~scheduler()
{ }

void scheduler::initialize(size_t nthreads, size_t deque_log_size)
{
	ASSERT(state == state_t::terminated, "Schdeler is already initialized");
	state = state_t::initializing;

	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();
	workers_count = nthreads;

	Debug() << "Starting scheduler with " << workers_count << " workers";
	Debug() << "Worker deque size: " << (1 << deque_log_size);

	workers = new worker *[workers_count];
	for (size_t i = 0; i < workers_count; ++i)
		workers[i] = new worker(deque_log_size);

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->fork();

	wait_workers_fork();

	state = state_t::initialized;
}

void scheduler::wait_workers_fork()
{
	for (size_t i = 1; i < workers_count; ++i) {
		if (workers[i]->ready())
			continue;

		do {
			Debug() << "Wating for worker #" << i << " to start";
			std::this_thread::yield();
		} while (!workers[i]->ready());
	}
}

void scheduler::wait_until_initilized()
{
	Debug() << "Wating for others workers to start";

	ASSERT(state == state_t::initializing || state_t::initialized,
			"Incorrect scheduler state");

	while (load_consume(state) == state_t::initializing) {
		std::this_thread::yield();
	}

	ASSERT(state != state_t::initializing, "Incorrect scheduler state");
}

void scheduler::terminate()
{
	ASSERT(state == state_t::initialized,
			"Incorrect scheduler state");
	Debug() << "Terminating";
	state = terminating;

// #if STACCATO_STATISTICS
// 	statistics::terminate();
// #endif // STACCATO_STATISTICS

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->join();

	for (size_t i = 0; i < workers_count; ++i)
		delete workers[i];

	delete []workers;

	state = terminated;

	Debug() << "Terminated";
}

void scheduler::spawn_and_wait(task *t)
{
	ASSERT(workers != nullptr, "Task Pool is not initialized");
	ASSERT(workers[0] != nullptr, "Task Pool is not initialized");

#if STACCATO_DEBUG
	ASSERT(t->get_state() == task::initializing,
			"Incorrect task state: " << t->get_state());
	t->set_state(task::taken);
#endif

	workers[0]->task_loop(t, t);
}

worker *scheduler::get_victim(worker *thief)
{
	ASSERT(workers_count != 0, "Stealing when scheduler has a single worker");

	size_t n = workers_count;
	size_t i = xorshift_rand() % n;
	auto victim = workers[i];
	if (victim == thief) {
		i++;
		i %= n;
		victim = workers[i];
	}

	return victim;
}

} // namespace stacccato
