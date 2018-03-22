#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

void fib_seq(size_t n, unsigned long *sum)
{
	if (n <= 2) {
		*sum = 1;
		return;
	}

	unsigned long x;
	fib_seq(n - 1, &x);

	unsigned long y;
	fib_seq(n - 2, &y);

	*sum = x + y;

	return;
}

int main(int argc, char *argv[])
{
	size_t n = 40;
	unsigned long answer;

	if (argc >= 3)
		n = atoi(argv[2]);

	auto start = system_clock::now();

	fib_seq(n, &answer);

	auto stop = system_clock::now();

	cout << "Scheduler:  sequential\n";
	cout << "Benchmark:  fib\n";
	cout << "Threads:    " << 0 << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
