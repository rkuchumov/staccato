#include <iostream>
#include <chrono>
#include <thread>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace chrono;
using namespace tbb;

class FibTask: public task
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

	task *execute() {
		if (n <= 2) {
			*sum = 1;
			return nullptr;
		}

		long x, y;
		FibTask &a = *new(allocate_child()) FibTask(n - 1, &x);
		FibTask &b = *new(allocate_child()) FibTask(n - 2, &y);

		set_ref_count(3);

		spawn(a);
		spawn(b);

		wait_for_all();

		*sum = x + y;

		return nullptr;
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

	task_scheduler_init scheduler(nthreads);

	auto root = new(task::allocate_root()) FibTask(n, &answer);

	task::spawn_root_and_wait(*root);

	scheduler.terminate();

	auto stop = system_clock::now();

	cout << "Scheduler:  tbb\n";
	cout << "Benchmark:  fib\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
