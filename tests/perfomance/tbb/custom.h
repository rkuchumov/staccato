#ifndef TBB_CUSTOM_H
#define TBB_CUSTOM_H

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../taskexec.h"

class TbbCustomTask: public tbb::task
{
public:
	TbbCustomTask(int breadth_, int depth_):
		depth(depth_), breadth(breadth_)
	{ }

	~TbbCustomTask() { }

	task *execute() {
		if (depth == 0)
			return NULL;

        set_ref_count(breadth + 1);

		for (int j = 0; j < breadth; j++) {
			TbbCustomTask &t = *new(allocate_child()) TbbCustomTask(breadth, depth - 1);
			spawn(t);
		}

		wait_for_all();

		return NULL;
	}

private:
	int depth;
	int breadth;
};

class TbbCustomTaskExec: public TaskExec
{
public:
	TbbCustomTaskExec(unsigned breadth_, unsigned depth_,
			size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size), breadth(breadth_), depth(depth_)
	{ 
		scheduler_init = new tbb::task_scheduler_init(nthreads);
	}

	const char *test_name() {
		return "TBB_Custom";
	}

	~TbbCustomTaskExec() {
		delete scheduler_init;
	}

	void run() {
		TbbCustomTask &root = *new(tbb::task::allocate_root()) TbbCustomTask(breadth, depth);
		tbb::task::spawn_root_and_wait(root);
	}

private:
	unsigned breadth;
	unsigned depth;

	tbb::task_scheduler_init *scheduler_init; 
};

#endif /* end of include guard: TBB_CUSTOM_H */

