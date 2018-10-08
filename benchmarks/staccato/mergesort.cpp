#include <iostream>
#include <chrono>
#include <thread>

#include <task.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace chrono;
using namespace staccato;

typedef int elem_t;

size_t lenght = 0;
elem_t *data = nullptr;
elem_t *data_tmp = nullptr;
long sum_before = 0;

inline uint32_t xorshift_rand() {
	static uint32_t x = 2463534242;
	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

void generate_data(size_t n) {
	lenght = n;
	data = new elem_t[n];
	data_tmp = new elem_t[n];
	sum_before = 0;
	for (size_t i = 0; i < n; ++i) {
		data[i] = xorshift_rand() % (n / 2);
		sum_before += data[i];
	}
}

bool check() {
	long s = data[0];
	for (size_t i = 1; i < lenght; ++i) {
		s += data[i];
		if (data[i] > data[i])
			return false;
	}

	return sum_before == s;
}

class SortTask: public task<SortTask>
{
public:
	SortTask (size_t left, size_t right)
	: m_left(left)
	, m_right(right)
	{ }

	static const size_t cutoff = 8192;

	static int qsort_cmp(const void* a, const void* b)
	{
		elem_t arg1 = *(const elem_t*)a;
		elem_t arg2 = *(const elem_t*)b;
		return (arg1 > arg2) - (arg1 < arg2);
	}

	void execute() {
		if (m_right - m_left <= cutoff) {
			qsort(data + m_left, m_right - m_left, sizeof(elem_t), qsort_cmp);
			return;
		}

		if (m_right - m_left <= 1)
			return;

		size_t mid = (m_left + m_right) / 2;
		size_t l = m_left;
		size_t r = mid;

		spawn(new(child()) SortTask(m_left, mid));
		spawn(new(child()) SortTask(mid, m_right));

		wait();

		for (size_t i = m_left; i < m_right; i++) {
			if ((l < mid && r < m_right && data[l] < data[r]) || r == m_right) {
				data_tmp[i] = data[l];
				l++;
			} else if ((l < mid && r < m_right) || l == mid) {
				data_tmp[i] = data[r];
				r++;
			}
		}

		memcpy(data + m_left, data_tmp + m_left, (m_right - m_left) * sizeof(elem_t));
	}

private:
	size_t m_left;
	size_t m_right;
};

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

	generate_data(n);

	auto start = system_clock::now();

	{
		scheduler<SortTask> sh(2, nthreads);
		sh.spawn(new(sh.root()) SortTask(0, n));
		sh.wait();
	}

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  mergesort\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << check() << "\n";

	return 0;
}
