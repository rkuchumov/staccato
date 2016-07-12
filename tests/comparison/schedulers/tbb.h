#ifndef TBB_H
#define TBB_H

#include <vector>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "AbstractScheduler.h"

using namespace std;

class TbbTask: public tbb::task
{
public:

	void exec_and_wait(vector<TbbTask *> &list) {
		tbb::task::set_ref_count(list.size() + 1);

		for (auto t: list)
			tbb::task::spawn(*t);

		tbb::task::wait_for_all();
	}

	tbb::task *execute() {
		my_execute();
		return nullptr;
	}

	void *operator new(size_t sz, TbbTask *callee = nullptr) {
		if (callee == nullptr) {
			return ::operator new(sz, tbb::task::allocate_root());
		} else {
			return ::operator new(sz, callee->allocate_child());
		}
	}

	virtual void my_execute() = 0;
};

class TbbScheduler: public abstact_sheduler<TbbTask>
{
public:
	TbbScheduler(size_t nthreads = 0) {
		if (nthreads)
			scheduler_init = new tbb::task_scheduler_init(nthreads);
		else
			scheduler_init = new tbb::task_scheduler_init();
	}

	~TbbScheduler() {
		delete scheduler_init;
	}

	void runRoot(TbbTask *task) {
		tbb::task::spawn_root_and_wait(*task);
	}

private:
	tbb::task_scheduler_init *scheduler_init; 
};

#endif /* end of include guard: TBB_H */
