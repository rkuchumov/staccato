#include "utils.h"
#include "scheduler.h"

namespace staccato
{

std::atomic_bool scheduler::is_active(false);
std::thread **scheduler::workers;
size_t scheduler::workers_count = 0;

internal::task_deque *scheduler::pool;
thread_local internal::task_deque* scheduler::my_pool;

size_t scheduler::deque_log_size = 8;
size_t scheduler::tasks_per_steal = 1;

#if STACCATO_DEBUG
thread_local size_t scheduler::my_id = 0;
#endif // STACCATO_DEBUG

scheduler::scheduler()
{

}

scheduler::~scheduler()
{
	// TODO: call terminate or something
}

void scheduler::initialize(size_t nthreads)
{
	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();

	check_paramters();

	internal::task_deque::deque_log_size = deque_log_size;
	internal::task_deque::tasks_per_steal = tasks_per_steal;
	pool = new internal::task_deque[nthreads];
	my_pool = pool;

	workers_count = nthreads - 1;

#if STACCATO_DEBUG
	my_id = 0;
	std::cerr << "Scheduler is working in debug mode\n";
#endif // STACCATO_DEBUG

#if STACCATO_STATISTICS
	internal::statistics::initialize();
#endif // STACCATO_STATISTICS

	is_active = true;

	if (workers_count != 0) {
		workers = new std::thread *[workers_count];

		for (size_t i = 0; i < workers_count; i++)
			workers[i] = new std::thread(initialize_worker, i + 1);
	}
}

void scheduler::terminate()
{
	ASSERT(is_active, "Task schedulter is not initializaed yet");

	is_active = false;

#if STACCATO_STATISTICS
	internal::statistics::terminate();
#endif // STACCATO_STATISTICS

	for (size_t i = 0; i < workers_count; i++)
		workers[i]->join();
}

void scheduler::check_paramters()
{
	if (deque_log_size == 0 || deque_log_size > 31) {
		deque_log_size = 8;
		std::cerr << "Incorrect deque size. Restored to 8\n";
	}

	if (tasks_per_steal == 0) {
		tasks_per_steal = 1;
		std::cerr << "Incorrect number of tasks per steal. Restored to 1\n";
	}
}


void scheduler::initialize_worker(size_t id)
{
	ASSERT(id > 0, "Worker #0 is master");

#if STACCATO_STATISTICS
	internal::statistics::set_worker_id(id);
#endif // STACCATO_STATISTICS

#if STACCATO_DEBUG
	my_id = id;
#endif // STACCATO_DEBUG

	my_pool = &pool[id];

	task_loop(nullptr);
}

void scheduler::task_loop(task *parent)
{
	ASSERT(parent != nullptr || (parent == nullptr && my_pool != pool),
			"Root task must be executed only on master");

	task *t = nullptr;

	while (true) {
		while (true) { // Local tasks loop
			if (t) {
#if STACCATO_DEBUG
				ASSERT(t->state == task::ready, "Task is already taken, state: " << t->state);
				t->state = task::executing;
#endif // STACCATO_DEBUG
				t->execute();
#if STACCATO_DEBUG
				ASSERT(t->state == task::executing, "");
				ASSERT(t->subtask_count == 0,
						"Task still has subtaks after it has been executed");
#endif // STACCATO_DEBUG

				if (t->parent != nullptr)
					dec_relaxed(t->parent->subtask_count);

				delete t;
			}

			if (parent != nullptr && load_relaxed(parent->subtask_count) == 0)
				return;

			t = my_pool->take();

			if (!t)
				break;
		} 

		if (parent == nullptr && !load_relaxed(is_active))
			return;

		t = steal_task();
	} 

	ASSERT(false, "Must never get there");
}

void scheduler::spawn(task *t)
{
	ASSERT(my_pool != nullptr, "Task Pool is not initialized");
	my_pool->put(t);
}

task *scheduler::steal_task()
{
	ASSERT(workers_count != 0, "Stealing when scheduler has a single worker");

	size_t n = workers_count + 1; // +1 for master
	size_t victim_id = internal::my_rand() % n;
	internal::task_deque *victim_pool = &pool[victim_id];
	if (victim_pool == my_pool)
		victim_pool = &pool[(victim_id + 1) % n];

	ASSERT(victim_pool != nullptr, "Stealing from NULL deque");
	ASSERT(victim_pool != my_pool, "Stealing from my own deque");

	return victim_pool->steal(my_pool);
}

} // namespace stacccato
