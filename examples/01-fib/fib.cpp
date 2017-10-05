#include <iostream>
#include <atomic>

#include <task.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace staccato;

class FibTask: public task
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

	void execute() {
		if (n <= 2) {
			*sum = 1;
			return;
		}

		long x, y;
		FibTask *a = new FibTask(n - 1, &x);
		FibTask *b = new FibTask(n - 2, &y);

		spawn(a);
		spawn(b);

		wait();

		delete a;
		delete b;

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

int main(int argc, char *argv[])
{
	unsigned n = 20;
	long answer;

	if (argc == 2) {
		n = atoi(argv[1]);
	}

	scheduler::initialize();

	FibTask root(n, &answer);
	scheduler::spawn(&root);
	scheduler::wait();

	cout << "fib(" << n << ") = " << answer << "\n";
	if (answer == fib(n))
		cout << "OK\n";
	else
		cout << "wrong\n";

	scheduler::terminate();
	cout << "done\n";

	return 0;
}
