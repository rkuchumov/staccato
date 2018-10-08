#include <iostream>
#include <chrono>
#include <thread>

#include <staccato/task.hpp>
#include <staccato/scheduler.hpp>

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

void seq_matmul(size_t left, size_t right)
{
	if (right - left <= 1)
		return;

	size_t mid = (left + right) / 2;
	size_t l = left;
	size_t r = mid;

	seq_matmul(left, mid);
	seq_matmul(mid, right);

	for (size_t i = left; i < right; i++) {
		if ((l < mid && r < right && data[l] < data[r]) || r == right) {
			data_tmp[i] = data[l];
			l++;
		} else if ((l < mid && r < right) || l == mid) {
			data_tmp[i] = data[r];
			r++;
		}
	}

	memcpy(data + left, data_tmp + left, (right - left) * sizeof(elem_t));
}

int main(int argc, char *argv[])
{
	size_t n = 8e7;

	if (argc >= 3)
		n = atoi(argv[2]);

	generate_data(n);

	auto start = system_clock::now();

	seq_matmul(0, n);

	auto stop = system_clock::now();

	cout << "Scheduler:  sequential\n";
	cout << "Benchmark:  mergesort\n";
	cout << "Threads:    " << 0 << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << check() << "\n";

	return 0;
}
