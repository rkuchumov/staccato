#ifndef CILK_H
#define CILK_H

#include <iostream>
#include <vector>

#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

#include "AbstractScheduler.h"

using namespace std;

class cilk_task
{
public:
	void exec_and_wait(vector<cilk_task *> &list) {
		for (auto t: list)
			cilk_spawn t->my_execute();

		// Proper way, but not "fair" comared to others:
		// for (size_t i = 1; i < list.size(); i++)
		// 	cilk_spawn list[i]->my_execute();
		// list[0]->my_execute();

		cilk_sync;
	}

	void *operator new(size_t sz, cilk_task * /* callee */ = nullptr) {
		return ::operator new(sz);
	}

	void operator delete(void *ptr) {
		return ::operator delete(ptr);
	}

	virtual void my_execute() = 0;
};

class cilk_sheduler: public abstact_sheduler<cilk_task>
{
public:
	cilk_sheduler(size_t nthreads = 0, size_t = 0) {
		string s = to_string(nthreads);
		__cilkrts_set_param("nworkers", s.c_str());
		__cilkrts_init();
	}

	~cilk_sheduler() { 
		__cilkrts_end_cilk();
	}

	void runRoot(cilk_task *task) {
		task->my_execute();
	}
};

#endif /* end of include guard: CILK_H */
