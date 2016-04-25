#include <iostream>

#include "scheduler.h"

using namespace std;
using namespace staccato;

class FibTask: public staccato::task
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

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

int main()
{
	unsigned n = 35;
	long answer;

	scheduler::initialize();

	FibTask root(n, &answer);
	root.execute();

	scheduler::terminate();

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
