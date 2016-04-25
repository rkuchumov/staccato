#ifndef FIB_H
#define FIB_H

#include "taskexec.h"

#include "scheduler.h"

class FibTask: public staccato::task
{
public:
	FibTask (int n_, long *sum_): 
		n(n_), sum(sum_)
	{ }

	~FibTask () { }

	void execute() {
		if (n < 2) {
			*sum = 1;
			return;
		}

		long x, y;
		FibTask *a = new FibTask(n - 1, &x);
		FibTask *b = new FibTask(n - 2, &y);

		spawn(a);
		spawn(b);

		wait_for_all();

		*sum = x + y;

		return;
	}

private:
	int n;
	long *sum;
};

class FibTaskExec: public TaskExec
{
public:
	FibTaskExec(unsigned n_, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size), n(n_)
	{
		if (steal_size > 1)
			staccato::scheduler::deque_log_size = 11;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	const char *test_name() {
		return "Fib";
	}

	~FibTaskExec() {
		staccato::scheduler::terminate();
	}

	void run() {
		FibTask root(n, &answer);
		root.execute();
	}

	bool check() {
		return answer == fib_seq(n);
	}

private:
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

#endif /* end of include guard: FIB_H */
