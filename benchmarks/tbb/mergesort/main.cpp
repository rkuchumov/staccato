#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace chrono;
using namespace tbb;

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

class SortTask: public task
{
public:
	SortTask (size_t left, size_t right)
	: m_left(left)
	, m_right(right)
	{ }

	task *execute() {
		if (m_right - m_left <= 1)
			return nullptr;

		size_t mid = (m_left + m_right) / 2;
		size_t l = m_left;
		size_t r = mid;

		SortTask &a = *new(allocate_child()) SortTask(m_left, mid);
		SortTask &b = *new(allocate_child()) SortTask(mid, m_right);

		set_ref_count(3);

		spawn(a);
		spawn(b);

		wait_for_all();

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

		return nullptr;
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

	task_scheduler_init scheduler(nthreads);

	auto root = new(task::allocate_root()) SortTask(0, n);

	task::spawn_root_and_wait(*root);

	scheduler.terminate();

	auto stop = system_clock::now();

	cout << "Scheduler:  tbb\n";
	cout << "Benchmark:  mergesort\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << check() << "\n";

	return 0;
}
