#include <iostream>
#include <atomic>

#include <task.hpp>
#include <scheduler.hpp>

#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace staccato;

class FibTask: public task<FibTask>
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
	size_t n = 20;
	long answer; 
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	{
		scheduler<FibTask> sh(nthreads, 2, n);
		sh.spawn(new(sh.root()) FibTask(n, &answer));
		sh.wait();
	}

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
