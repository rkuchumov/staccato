#include <iostream>
#include <chrono>
#include <thread>

#include <omp.h>

using namespace std;
using namespace std::chrono;

void fib(int n, unsigned long *sum)
{
	if (n <= 2) {
		*sum = 1;
		return;
	}

	unsigned long x;
#pragma omp task shared(n, x)
	fib(n - 1, &x);

	unsigned long y;
#pragma omp task shared(n, y)
	fib(n - 2, &y);

#pragma omp taskwait

	*sum = x + y;
}

void test(int n, unsigned long *sum)
{
#pragma omp task shared(n, sum)
		fib(n, sum);
#pragma omp taskwait
}

int main(int argc, char *argv[])
{
	size_t n = 40;
	unsigned long answer;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	auto start = system_clock::now();

	omp_set_dynamic(0);
	omp_set_num_threads(nthreads);

#pragma omp parallel shared(n, answer)
#pragma omp single
	test(n, &answer);

	auto stop = system_clock::now();

	cout << "Scheduler:  omp\n";
	cout << "Benchmark:  fib\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
