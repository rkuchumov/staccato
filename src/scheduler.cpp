#include "utils.h"
#include "scheduler.h"

task_deque *scheduler::pool;
std::thread **scheduler::workers;
thread_local task_deque* scheduler::my_pool = NULL;
bool scheduler::is_active = false;
size_t scheduler::workers_count = 0;

void scheduler::initialize(size_t nthreads)
{
	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();

	pool = new task_deque[nthreads];
	my_pool = pool;

	workers_count = nthreads - 1;

#ifndef NSTAT
	statistics::initialize();
#endif

	is_active = true;

	if (nthreads <= 1)
		return;

	workers = new std::thread *[workers_count];

	for (size_t i = 0; i < workers_count; i++)
		workers[i] = new std::thread(initialize_worker, i + 1);
}

void scheduler::terminate()
{
	ASSERT(is_active, "Task schedulter is not initializaed yet");

	is_active = false;

#ifndef NSTAT
	statistics::terminate();
#endif

	for (size_t i = 0; i < workers_count; i++)
		workers[i]->join();

#ifndef NDEBUG
	std::cerr << "scheduler was working in debug mode\n";
#endif
}

void scheduler::initialize_worker(size_t id)
{
	ASSERT(id > 0, "Worker #0 is master");

	my_pool = &pool[id];

	task_loop(NULL);
}

void scheduler::task_loop(task *parent)
{
	ASSERT(parent != NULL || (parent == NULL && my_pool != pool),
			"Root task must be executed only on master");

	task *t = NULL;

	while (true) {
		while (true) { // Local tasks loop
			if (t) {
#ifndef NDEBUG
				ASSERT(t->state == task::ready, "Task is already taken, state: " << t->state);
				t->state = task::executing;
#endif
				t->execute();
#ifndef NDEBUG
				ASSERT(t->state == task::executing, "");
				ASSERT(t->subtask_count == 0,
						"Task still has subtaks after it has been executed");
#endif

				if (t->parent != NULL)
					dec_relaxed(t->parent->subtask_count);

				delete t;
			}

			if (parent != NULL && load_relaxed(parent->subtask_count) == 0)
				return;

			t = my_pool->take();

			if (!t)
				break;
		} 

		if (parent == NULL && !is_active)
			return;

		t = steal_task();
	} 

	ASSERT(false, "Must never get there");
}

void scheduler::spawn(task *t)
{
	ASSERT(my_pool != NULL, "Task Pool is not initialized");
	my_pool->put(t);
}

task *scheduler::steal_task()
{
	ASSERT(workers_count != 0, "Stealing when scheduler has a single worker");

	size_t n = workers_count + 1; // +1 for master
	size_t victim_id = rand() % n;
	task_deque *victim_pool = &pool[victim_id];
	if (victim_pool == my_pool)
		victim_pool = &pool[(victim_id + 1) % n];

	ASSERT(victim_pool != NULL, "Stealing from NULL deque");
	ASSERT(victim_pool != my_pool, "Stealing from my own deque");
	task *t = victim_pool->steal();

	return t;
}
