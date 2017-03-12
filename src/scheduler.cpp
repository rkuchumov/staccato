#include "utils.h"
#include "scheduler.h"
#include "worker.h"
#include "task.h"

namespace staccato
{
using namespace internal;

std::atomic_bool scheduler::is_active(false);
worker **scheduler::workers;
size_t scheduler::workers_count(0);

scheduler::scheduler()
{ }

scheduler::~scheduler()
{ }

void scheduler::initialize(size_t nthreads, size_t deque_log_size)
{
	ASSERT(is_active == false, "Schdeler is already initialized");

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

	is_active = true;
}

void scheduler::wait_workers_fork()
{
	while (!load_consume(is_active)) {
		std::this_thread::yield();
	}
}

void scheduler::terminate()
{
	ASSERT(is_active, "Task schedulter is not initializaed yet");
	Debug() << "Terminating scheduler\n";

	is_active = false;

// #if STACCATO_STATISTICS
// 	statistics::terminate();
// #endif // STACCATO_STATISTICS

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->join();

	for (size_t i = 0; i < workers_count; ++i)
		delete workers[i];

	delete []workers;
}

void scheduler::spawn_and_wait(task *t)
{
	ASSERT(workers != nullptr, "Task Pool is not initialized");
	ASSERT(workers[0] != nullptr, "Task Pool is not initialized");

	ASSERT(t->get_state() == task::initializing,
			"Incorrect task state: " << t->get_state_str());
	t->set_state(task::taken);
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
