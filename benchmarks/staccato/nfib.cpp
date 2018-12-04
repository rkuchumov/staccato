#include <iostream>
#include <chrono>
#include <thread>

#include <task.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace chrono;
using namespace staccato;

int N;

class NFibTask: public task<NFibTask>
{
public:
	NFibTask (int n_, unsigned long *sum_): n(n_), sum(sum_)
	{ }

	void execute() {
		if (n <= N) {
			*sum = 1;
			return;
		}

		unsigned long *ans = new unsigned long[N];

		for (int i = 0; i < N; ++i)
			spawn(new(child()) NFibTask(n - i - 1, ans + i));

		wait();

		*sum = 0;

		for (int i = 0; i < N; ++i)
			*sum += ans[i];

		delete[] ans;

		return;
	}

private:
	int n;
	unsigned long *sum;
};

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

	auto start = system_clock::now();

	{
		scheduler<NFibTask> sh(N, nthreads);
		sh.spawn(new(sh.root()) NFibTask(n, &answer));
		sh.wait();
	}

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  nfib-glibc\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << N << " " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
