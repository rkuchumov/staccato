#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include <task.hpp>
#include <scheduler.hpp>
#include <unistd.h>

using namespace std;
using namespace chrono;
using namespace staccato;

typedef double elem_t;

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
		if (m_len <= 4) {
			long n = 0;
			for (size_t i = 0; i < m_len; ++i) {
				D[i] = A[i] * B[i] + C[i];
				if (D[i] == 10)
					n++;
			}

			*m_cnt = n;
			return;
		}

		size_t l = m_start;
		size_t ln = m_len / 2;
		size_t r = m_start + ln;
		size_t rn = m_len - ln;


		long x;
		spawn(new(child()) CounterTask(l, ln, &x));

		long y;
		spawn(new(child()) CounterTask(r, rn, &y));

		wait();

		*m_cnt = x + y;

		return;
	}

	static elem_t *A;
	static elem_t *B;
	static elem_t *C;
	static elem_t *D;

private:
	size_t m_start;
	size_t m_len;
	long *m_cnt;
};

elem_t *CounterTask::A;
elem_t *CounterTask::B;
elem_t *CounterTask::C;
elem_t *CounterTask::D;

int main(int argc, char *argv[])
{
	size_t logn = 8;
	size_t nthreads = 0;
	bool single_node = false;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		logn = atoi(argv[2]);
	if (argc >= 4)
		single_node = atoi(argv[3]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	size_t n = 1 << logn;
	CounterTask::A = new elem_t[n];
	CounterTask::B = new elem_t[n];
	CounterTask::C = new elem_t[n];
	CounterTask::D = new elem_t[n];

	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);

	unsigned nr_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NUMANODE);
	cout << "NUMA nodes: " << nr_nodes << "\n";

	unsigned node_size = sizeof(elem_t) * n / nr_nodes;

	for (unsigned n = 0; n < nr_nodes; ++n) {
		hwloc_obj_t node = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 0);

		size_t offset = n * node_size;

		hwloc_set_area_membind(topology, CounterTask::A + offset, node_size, node->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
		hwloc_set_area_membind(topology, CounterTask::B + offset, node_size, node->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
		hwloc_set_area_membind(topology, CounterTask::C + offset, node_size, node->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
		hwloc_set_area_membind(topology, CounterTask::D + offset, node_size, node->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	}

	cout << "Haystack:     " << n*sizeof(unsigned) / (1024*1024) << " Mb\n";

	std::cerr << "init\n";
	for (size_t i = 0; i < n; ++i) {
		CounterTask::A[i] = xorshift_rand();
		CounterTask::B[i] = xorshift_rand();
		CounterTask::C[i] = xorshift_rand();
	}

	long answer = 0;
	auto start = system_clock::now();

	{
		CounterTask root(0, n, &answer);
		scheduler<CounterTask> sh(2, nthreads, 1, single_node);
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
