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

		long x;
		spawn(new(child()) FibTask(n - 1, &x));

		long y;
		spawn(new(child()) FibTask(n - 2, &y));

		wait();

		*sum = x + y;

		return;
	}

private:
	int n;
	long *sum;
};

int main(int argc, char *argv[])
{
	unsigned n = 20;
	long answer; 

	if (argc == 2) {
		n = atoi(argv[1]);
	}

	scheduler::initialize(sizeof(FibTask));

	scheduler::spawn(new(scheduler::root()) FibTask(n, &answer));
	scheduler::wait();

	scheduler::terminate();

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
