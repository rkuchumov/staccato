#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include <cilk/cilk.h>
#include <cilk/cilk_api.h> 

using namespace std;
using namespace chrono;

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

void mergesort(size_t left, size_t right) {
	if (right - left <= 1)
		return;

	size_t mid = (left + right) / 2;
	size_t l = left;
	size_t r = mid;

	cilk_spawn mergesort(left, mid);
	cilk_spawn mergesort(mid, right);

	cilk_sync;

	for (size_t i = left; i < right; i++) {
		if ((l < mid && r < right && data[l] < data[r]) || r == right) {
			data_tmp[i] = data[l];
			l++;
		} else if ((l < mid && r < right) || l == mid) {
			data_tmp[i] = data[r];
			r++;
		}
	}

	// TODO: prefetch?
	memcpy(data + left, data_tmp + left, (right - left) * sizeof(elem_t));

}
void test(size_t left, size_t right)
{
	cilk_spawn mergesort(left, right);
	cilk_sync;
}

int main(int argc, char *argv[])
{
	size_t n = 8e7;
	const char *nthreads = nullptr;

	if (argc >= 2)
		nthreads = argv[1];
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = to_string(thread::hardware_concurrency()).c_str();

	generate_data(n);

	__cilkrts_end_cilk(); 

	auto start = system_clock::now();

	if (__cilkrts_set_param("nworkers", nthreads) != 0) {
		cerr << "Failed to set worker count\n";
		exit(EXIT_FAILURE);
	}

	__cilkrts_init();

	test(0, n);

	__cilkrts_end_cilk(); 

	auto stop = system_clock::now();

	cout << "Scheduler:  cilk\n";
	cout << "Benchmark:  mergesort\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << check() << "\n";

	return 0;
}
