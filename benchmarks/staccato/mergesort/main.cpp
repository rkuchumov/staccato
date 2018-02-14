#include <iostream>
#include <chrono>
#include <thread>

#include <staccato/task.hpp>
#include <staccato/scheduler.hpp>

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


class SortTask: public task<SortTask>
{
public:
	SortTask (size_t left, size_t right)
	: m_left(left)
	, m_right(right)
	{ }

	struct elem_t {
		int x;
		int t;
	};

	static void generate_data(size_t n) {
		size = n;
		data = new elem_t[n];
		for (size_t i = 0; i < n; ++i)
			data[i].x = xorshift_rand() % (n / 2);
	}

	static long get_sum() {
		long s = 0;
		for (size_t i = 0; i < size; ++i)
			s += data[i].x;
		return s;
	}

	void execute() {
		if (m_right - m_left <= 1)
			return;

		size_t mid = (m_left + m_right) / 2;
		size_t l = m_left;
		size_t r = mid;

		spawn(new(child()) SortTask(m_left, mid));
		spawn(new(child()) SortTask(mid, m_right));

		wait();

		for (size_t i = m_left; i < m_right; i++) {
			if ((l < mid && r < m_right && data[l].x < data[r].x) || r == m_right) {
				data[i].t = data[l].x;
				l++;
			} else if ((l < mid && r < m_right) || l == mid) {
				data[i].t = data[r].x;
				r++;
			}
		}

		// TODO: prefetch?
		for (size_t i = m_left; i < m_right; ++i)
			data[i].x = data[i].t;
	}

private:
	static size_t size;
	static elem_t *data;

	size_t m_left;
	size_t m_right;
};

size_t SortTask::size = 0;
SortTask::elem_t *SortTask::data;

int main(int argc, char *argv[])
{
	size_t n = 8e7;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	SortTask::generate_data(n);

	auto start = system_clock::now();

	{
		scheduler<SortTask> sh(nthreads, 2);
		sh.spawn(new(sh.root()) SortTask(0, n));
		sh.wait();
	}

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  fib\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << SortTask::get_sum() << "\n";

	return 0;
}
