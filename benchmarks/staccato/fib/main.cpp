#include <iostream>
#include <chrono>
#include <thread>

#include <staccato/task.hpp>
#include <staccato/scheduler.hpp>

using namespace std;
using namespace chrono;
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

int main(int argc, char *argv[])
{
	size_t n = 40;
	long answer;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	auto start = system_clock::now();

	scheduler::initialize(nthreads);

	auto root = new FibTask(n, &answer);
	scheduler::spawn_and_wait(root);
	delete root;

	scheduler::terminate();

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  fib\n";
	cout << "Threads:  " << nthreads << "\n";
	cout << "Time(us): " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:    " << n << "\n";
	cout << "Output:   " << answer << "\n";

	return 0;
}
