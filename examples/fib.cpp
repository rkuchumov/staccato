#include <iostream>
#include <atomic>

#include "scheduler.h"

using namespace std;
using namespace staccato;

class FibTask: public task
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{
		cerr << "ctor: " << n << "\n";
	}

	void execute() {
		cerr << "exec: " << n << "\n";
		if (n <= 2) {
			*sum = 1;
			return;
		}

		long x, y;
		FibTask *a = new FibTask(n - 1, &x);
		FibTask *b = new FibTask(n - 2, &y);

		spawn(a);
		spawn(b);

		wait_for_all();

		// delete a;
		// delete b;

		*sum = x + y;

		return;
	}

private:
	int n;
	long *sum;
};

int fib(int n)
{
    int a = 1, b = 1;
    for (int i = 3; i <= n; i++) {
        int c = a + b;
        a = b;
        b = c;
    }           
    return b;
}

int main()
{
	unsigned n = 20;
	long answer;

	scheduler::initialize(4);

	FibTask root(n, &answer);
	scheduler::spawn_and_wait(&root);

	cout << "fib(" << n << ") = " << answer << "\n";
	if (answer == fib(n))
		cout << "OK\n";
	else
		cout << "wrong\n";

	scheduler::terminate();

	return 0;
}
