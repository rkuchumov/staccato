#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

int N;

void fib_seq(int n, unsigned long *sum)
{
	if (n <= N) {
		*sum = 1;
		return;
	}

	unsigned long* ans = new unsigned long[N];

	for (int i = 0; i < N; ++i)
		fib_seq(n - i - 1, ans + i);

	for (int i = 0; i < N; ++i)
		*sum += ans[i];

	delete[] ans;

	return;
}

int main(int argc, char *argv[])
{
	size_t n = 40;
	unsigned long answer;

	if (argc >= 3)
		N = atoi(argv[2]);
	if (argc >= 4)
		n = atoi(argv[3]);

	auto start = system_clock::now();

	fib_seq(n, &answer);

	auto stop = system_clock::now();

	cout << "Scheduler:  sequential\n";
	cout << "Benchmark:  nfib-glibc\n";
	cout << "Threads:    " << 0 << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << N << " " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
