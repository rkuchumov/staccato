#ifndef CUSTOM_H
#define CUSTOM_H

#include "taskexec.h"

#include "scheduler.h"

class CustomTask: public staccato::task
{
public:
	CustomTask(int breadth_, int depth_):
		depth(depth_), breadth(breadth_)
	{ }

	~CustomTask() { }

	void execute() {
		if (depth == 0)
			return;

		for (int j = 0; j < breadth; j++) {
			CustomTask *t = new CustomTask(breadth, depth - 1);
			spawn(t);
		}

		wait_for_all();
	}

private:
	int depth;
	int breadth;
};

class CustomTaskExec: public TaskExec
{
public:
	CustomTaskExec(unsigned breadth_, unsigned depth_,
			size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size), breadth(breadth_), depth(depth_)
	{ 
		if (steal_size > 1)
			staccato::scheduler::deque_log_size = 11;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	const char *test_name() {
		return "Custom";
	}

	~CustomTaskExec() {
		staccato::scheduler::terminate();
	}

	void run() {
		CustomTask root(breadth, depth);
		root.execute();
	}

private:
	unsigned breadth;
	unsigned depth;
};

#endif /* end of include guard: CUSTOM_H */

