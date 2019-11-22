#include <iostream>
#include <chrono>
#include <thread>

#include <task.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace chrono;
using namespace staccato;

inline uint32_t xorshift_rand() {
	static uint32_t x = 2463534242;
	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

class CounterTask: public task<CounterTask>
{
public:
	CounterTask(size_t start, size_t len, long *cnt)
	: m_start(start)
	, m_len(len)
	, m_cnt(cnt)
	{ }

	void execute() {
		if (m_len <= 16) {
			long n = 0;
			for (size_t i = 0; i < m_len; ++i) {
				if (haystack[m_start + i] == needle)
					n++;
			}

			*m_cnt = n;
			return;
		}

		size_t l = start;
		size_t ln = m_len / 2;
		size_t r = start + ln;
		size_t rn = m_len - r;

		long x;
		spawn(new(child()) ConterTask(l, ln, &x));

		long y;
		spawn(new(child()) ConterTask(r, rn, &y));

		wait();

		*m_cnt = x + y;

		return;
	}

	static unsigned needle;
	static unsigned *haystack;

private:
	size_t m_start;
	size_t m_len;
	long *m_cnt;
};

unsigned CounterTask::needle;
unsigned *CounterTask::haystack;


int main(int argc, char *argv[])
{
	size_t logn = 8;
	unsigned long answer;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	size_t n = 1 << logn;
	CounterTask::haystack = new unsigned[n];

	for (size_t i = 0; i < n; ++i)
		CounterTask::haystack[i] = xorshift_rand();

	CounterTask::needle = xorshift_rand();

	long answer = 0;
	auto start = system_clock::now();

	{
		CounterTask root(0, n, answer);
		scheduler<CounterTask> sh(2, nthreads);
		sh.spawn_and_wait(&root);
	}

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  counter\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << logn << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
