#include "utils.h"
#include "scheduler.h"

namespace staccato
{

std::atomic_bool scheduler::m_is_active(false);
internal::worker **scheduler::workers;
size_t scheduler::workers_count;

// #if STACCATO_DEBUG
// thread_local size_t scheduler::my_id = 0;
// #endif // STACCATO_DEBUG

scheduler::scheduler()
{ }

scheduler::~scheduler()
{ }

void scheduler::initialize(size_t nthreads, size_t deque_log_size)
{
	ASSERT(m_is_active == false, "Schdeler is already initialized");

	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();
	workers_count = nthreads;

	workers = new internal::worker *[workers_count];
	for (size_t i = 0; i < workers_count; ++i)
		workers[i] = new internal::worker(deque_log_size);

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->fork();

	m_is_active = true;

}

bool scheduler::is_active()
{ return m_is_active;
}

void scheduler::terminate()
{
	ASSERT(m_is_active, "Task schedulter is not initializaed yet");

	m_is_active = false;

// #if STACCATO_STATISTICS
// 	internal::statistics::terminate();
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

	// TODO: name this worker
	// workers[0]->enqueue(t);
	//
	ASSERT(t->parent == nullptr, "parent of a root should be null");

	workers[0]->task_loop(t, t);
	// workers[0]->task_loop();
}

internal::worker *scheduler::get_victim(internal::worker *thief)
{
	ASSERT(workers_count != 0, "Stealing when scheduler has a single worker");

	size_t n = workers_count;
	size_t i = internal::xorshift_rand() % n;
	auto victim = workers[i];
	if (victim == thief) {
		i++;
		i %= n;
		victim = workers[i];
	}

	// std::cerr << n << " stealing " << workers[0] << " " << workers[1] << std::endl;

	// ASSERT(victim_pool != nullptr, "Stealing from NULL deque");
	// ASSERT(victim_pool != my_pool, "Stealing from my own deque");
	return victim;
}

} // namespace stacccato
