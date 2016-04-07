#include "utils.h"
#include "scheduler.h"

task_deque *scheduler::pool;
std::thread **scheduler::workers;
unsigned thread_local scheduler::me;
bool scheduler::is_active = false;
size_t scheduler::workers_count = 0;

void scheduler::initialize(size_t nthreads)
{
	ASSERT(nthreads > 0, "Number of worker threads must be greater than 0");

	pool = new task_deque[nthreads];

	me = 0;

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
	std::cerr << "scheduler debug mode\n";
#endif
}

void scheduler::initialize_worker(size_t id)
{
	me = id;

	task_loop(NULL);
}

void scheduler::task_loop(task *parent)
{
	assert(parent != NULL || (parent == NULL && me != 0));

	task *t = NULL;

	while (true) {
		while (true) {
			if (t) {
#ifndef NDEBUG
				ASSERT(t->state == task::ready, me << " task is already taken, state: " << t->state);
				t->state = task::executing;
#endif
				t->execute();
#ifndef NDEBUG
				assert(t->state == task::executing);
#endif
				ASSERT(t->get_subtask_count() == 0, "Task still has subtaks after it has been executed");

				if (t->parent() != NULL)
					t->parent()->decrement_subtask_count();

				delete t;
			}

			if (parent != NULL && parent->get_subtask_count() == 0)
				return;

			t = pool[me].take();

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
	pool[me].put(t);
}

task *scheduler::steal_task()
{
	if (workers_count == 0)
		return NULL;

	size_t n = workers_count + 1;
	size_t victim = rand() % n;
	if (victim == me)
		victim = (victim + 1) % n;

	ASSERT(victim != me, "Stealing from my own deque");
	task *t = pool[victim].steal();

	return t;
}
