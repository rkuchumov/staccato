#ifndef OPENMP_H
#define OPENMP_H

#include <iostream>
#include <iostream>
#include <vector>

using namespace std;

#include "AbstractScheduler.h"

class openmp_task
{
public:
	void exec_and_wait(vector<openmp_task *> &list) {
		for (auto t: list)
#pragma omp task 
			t->my_execute();

#pragma omp taskwait
	}

	void execute() {
		my_execute();
	}

	void *operator new(size_t sz, openmp_task * /* callee */ = nullptr) {
		return ::operator new(sz);
	}

	void operator delete(void *ptr) {
		return ::operator delete(ptr);
	}

	virtual void my_execute() = 0;
};

class openmp_sheduler: public abstact_sheduler<openmp_task>
{
private:
	size_t nthreads;

public:
	openmp_sheduler(size_t nthreads_ = 0):
		nthreads(nthreads_) 
	{ }

	~openmp_sheduler() { }

	void runRoot(openmp_task *task) {
#pragma omp parallel shared(task)
#pragma omp single
		task->execute();
	}
};

#endif /* end of include guard: OPENMP_H */

