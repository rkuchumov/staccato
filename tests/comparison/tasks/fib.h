#ifndef FIB_H
#define FIB_H

#include <vector>
#include <string>

#include "../test_suite.h"

using namespace std;

template<typename task, bool no_del = false>
class fib_task: public task
{
public:
	fib_task (int n_, long *sum_): n(n_), sum(sum_) { }
	~fib_task () { }

	void my_execute() {
		if (n < 2) {
			*sum = 1;
			return;
		}

		long x, y;

		fib_task<task, no_del> *a = new(this) fib_task<task, no_del>(n - 1, &x);
		fib_task<task, no_del> *b = new(this) fib_task<task, no_del>(n - 2, &y);

		vector<task *> l;
		l.push_back(a);
		l.push_back(b);

		task::exec_and_wait(l);

		*sum = x + y;

		if (no_del)
			return;

		delete a;
		delete b;
	}

private:

	int n;
	long *sum;
};

template<typename scheduler, typename task, bool no_del = false>
class fib_test: public abstract_test
{
public:
	fib_test(string name_, int n_ = 35, size_t nthreads = 0, size_t k = 0) 
		: abstract_test(name_), n(n_)
	{
		sh = new scheduler(nthreads, k);
	}

	~fib_test() {
		delete sh;
	}

	void set_up() {
		root = new fib_task<task, no_del> (n, &ans);
	}

	void iteration() {
		sh->runRoot(root);
	}

	bool check() {
		return fib_seq() == ans;
	}

	void break_down() {
		ans = 0;
		if (!no_del)
			delete root;
	}

	unsigned long get_steals() {
		return sh->get_steals();
	}

	double get_delay() {
		return sh->get_delay();
	}

private:
	scheduler *sh;

	int n;
	long ans;
	fib_task<task, no_del> *root;
	
	long fib_seq() {
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
};

#endif /* end of include guard:  */
