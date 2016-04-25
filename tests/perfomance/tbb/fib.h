#ifndef TBB_FIB_H
#define TBB_FIB_H

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../taskexec.h"

class TbbFibTask: public tbb::task
{
public:
	TbbFibTask (int n_, long *sum_): 
		n(n_), sum(sum_)
	{ }

	~TbbFibTask () { }

	task *execute() {
		if (n < 2) {
			*sum = 1;
			return NULL;
		}

        set_ref_count(3);

		long x, y;
		TbbFibTask &a = *new(allocate_child()) TbbFibTask(n - 1, &x);
		TbbFibTask &b = *new(allocate_child()) TbbFibTask(n - 2, &y);

		spawn(a);
		spawn(b);

		wait_for_all();

		*sum = x + y;

		return NULL;
	}

private:
	int n;
	long *sum;
};

class TbbFibTaskExec: public TaskExec
{
public:
	TbbFibTaskExec(unsigned n_, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size), n(n_)
	{
		scheduler_init = new tbb::task_scheduler_init(nthreads);
	}

	const char *test_name() {
		return "TBB_Fib";
	}

	~TbbFibTaskExec() {
		delete scheduler_init;
	}

	void run() {
		TbbFibTask &root = *new(tbb::task::allocate_root()) TbbFibTask(n, &answer);
		tbb::task::spawn_root_and_wait(root);
	}

	bool check() {
		return answer == fib_seq(n);
	}

private:
	tbb::task_scheduler_init *scheduler_init; 

	inline long fib_seq(unsigned n)
	{
		if (n < 2)
			return 1;

		long x = 1;
		long y = 1;
		long ans = 0;
		for (unsigned i = 2; i <= n; i++) {
			ans = x + y;
			x = y;
			y = ans;
		}

		return ans;
	}

	unsigned n;
	long answer;
};

#endif /* end of include guard: TBB_FIB_H */
