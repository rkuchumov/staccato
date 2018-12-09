#include <iostream>
#include <chrono>
#include <thread>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace chrono;
using namespace tbb;

int N;

class NFibTask: public task
{
public:
	NFibTask (int n_, unsigned long *sum_): n(n_), sum(sum_)
	{ }

	task *execute() {
		if (n <= N) {
			*sum = 1;
			return nullptr;
		}

		unsigned long* ans = new unsigned long[N];

		set_ref_count(N + 1);

		for (int i = 0; i < N; ++i)
		{
			NFibTask &b = *new(allocate_child()) NFibTask(n - i - 1, ans + i);
			spawn(b);
		}

		wait_for_all();

		*sum = 0;

		for (int i = 0; i < N; ++i)
			*sum += ans[i];

		delete[] ans;

		return nullptr;
	}

private:
	int n;
	unsigned long *sum;
};

void fib_seq(int n, unsigned long *sum) {
	if (n <= 2) {
		*sum = 1;
		return;
	}

	unsigned long x;
	fib_seq(n - 1, &x);

	unsigned long y;
	fib_seq(n - 2, &y);

	*sum = x + y;
}

int main(int argc, char *argv[])
{
	size_t n = 40;
	unsigned long answer;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		N = atoi(argv[2]);
	if (argc >= 4)
		n = atoi(argv[3]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	auto start_noshed = system_clock::now();

	if (nthreads == 1)
		fib_seq(n, &answer);

	auto stop_noshed = system_clock::now();
	auto noshed_time = duration_cast<microseconds>(stop_noshed - start_noshed).count();
	cout << "Seq time(us):   " << noshed_time << "\n";

	auto start = system_clock::now();

	task_scheduler_init scheduler(nthreads);

	auto root = new(task::allocate_root()) NFibTask(n, &answer);

	task::spawn_root_and_wait(*root);

	scheduler.terminate();

	auto stop = system_clock::now();

	auto shed_time = duration_cast<microseconds>(stop - start).count();

	cout << "Scheduler:  tbb\n";
	cout << "Benchmark:  nfib-glibc\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << N << " " << n << "\n";
	cout << "Output:     " << answer << "\n";

	if (nthreads == 1)
		cout << "Overhead:   " << static_cast<double>(shed_time) / noshed_time << "\n";

	return 0;
}
