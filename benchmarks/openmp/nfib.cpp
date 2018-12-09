#include <iostream>
#include <chrono>
#include <thread>

#include <omp.h>

using namespace std;
using namespace std::chrono;

int N;

void fib(int n, unsigned long *sum)
{
	if (n <= N) {
		*sum = 1;
		return;
	}

	unsigned long *ans = new unsigned long[N];

	for (int i = 0; i < N; ++i) {
#pragma omp task shared(n, x)
		fib(n - i - 1, ans + i);
	}

#pragma omp taskwait

	*sum = 0;

	for (int i = 0; i < N; ++i)
		*sum += ans[i];

	delete[] ans;
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
		N = atoi(argv[2]);
	if (argc >= 4)
		n = atoi(argv[3]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	auto start = system_clock::now();

	omp_set_dynamic(0);
	omp_set_num_threads(nthreads);

#pragma omp parallel shared(n, answer)
#pragma omp single
	test(n, &answer);

	auto stop = system_clock::now();

	const char* env_aff = std::getenv("KMP_AFFINITY");

	cout << "Scheduler:  omp\n";
	cout << "Benchmark:  fib\n";
	cout << "Affinity:   " << (env_aff ? env_aff : "none") << "\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
