#include "utils.h"
#include "scheduler.h"

bool scheduler::is_active = false;
std::thread **scheduler::workers;
size_t scheduler::workers_count = 0;

task_deque *scheduler::pool;
thread_local task_deque* scheduler::my_pool;

size_t scheduler::deque_log_size = 8;
size_t scheduler::tasks_per_steal = 1;

#ifndef NDEBUG
thread_local size_t scheduler::my_id = 0;
#endif

void scheduler::initialize(size_t nthreads)
{
	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();

	check_paramters();

	task_deque::deque_log_size = deque_log_size;
	task_deque::tasks_per_steal = tasks_per_steal;
	pool = new task_deque[nthreads];
	my_pool = pool;

#ifndef NSTAT
	statistics::initialize();
#endif

#ifndef NDEBUG
	my_id = 0;
#endif

	is_active = true;

	workers_count = nthreads - 1;
	if (workers_count == 0)
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

#ifndef NDEBUG
	my_id = id;
#endif

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

	return victim_pool->steal(my_pool);
}
