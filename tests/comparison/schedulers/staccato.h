#ifndef STACCATO_H
#define STACCATO_H

#include <iostream>
#include <vector>

using namespace std;

#include "AbstractScheduler.h"
#include "scheduler.h"

class staccato_task: private staccato::task
{
public:
	void exec_and_wait(vector<staccato_task *> &list) {
		for (auto t: list)
			staccato::task::spawn(t);

		staccato::task::wait_for_all();
	}

	void execute() {
		my_execute();
	}

	void set_ref_count(int) { }

	void *operator new(size_t sz, staccato_task * /* callee */ = nullptr) {
		return ::operator new(sz);
	}

	void operator delete(void *ptr) {
		return ::operator delete(ptr);
	}

	virtual void my_execute() = 0;
};

class staccato_scheduler: public abstact_sheduler<staccato_task>
{
public:
	staccato_scheduler(size_t nthreads = 0) {
		staccato::scheduler::initialize(nthreads);
	}

	~staccato_scheduler() {
		staccato::scheduler::terminate();
	}

	void runRoot(staccato_task *task) {
		task->execute();
	}
};

#endif /* end of include guard: STACCATO_H */
