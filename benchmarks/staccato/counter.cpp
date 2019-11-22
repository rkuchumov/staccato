#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include <task.hpp>
#include <scheduler.hpp>
#include <numaif.h>
#include <unistd.h>

using namespace std;
using namespace chrono;
using namespace staccato;

typedef std::atomic_uint elem_t;

inline uint32_t xorshift_rand() {
	static uint32_t x = 2463534242;
	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

void migrate_pages(void *ptr, size_t n, int node)
{
	long page_size = sysconf(_SC_PAGESIZE);

	if (n < page_size)
		return;

	size_t nr_pages = n / page_size; 

	void **pages = new void*[nr_pages];
	int *nodes = new int[nr_pages];
	int *status = new int[nr_pages]();

	long start = ((long) ptr) & -page_size;
	for (size_t i = 0; i < nr_pages; ++i) {
		nodes[i] = node;
		pages[i] = (void *)(start + i * page_size);
		status[i] = -EBUSY;
	}

	unsigned nr_moved = -1;
	while (nr_moved != 0 && nr_moved != nr_pages) {
		for (size_t i = 0; i < nr_pages; ++i) {
			if (status[i] != -EBUSY)
				pages[i] = nullptr;
		}

		long rc = move_pages(0, nr_pages, pages, nodes, status, 0);
		if (rc != 0)
			std::cerr << "move_pages error: " << strerror(errno) << "\n";

		nr_moved = 0;
		for (size_t i = 0; i < nr_pages; ++i) {
			if (status[i] == nodes[i]) {
				nr_moved++;
			}
		}


		std::clog << "moved " << nr_moved << "/" << nr_pages << "\n";
	}

	delete []pages;
	delete []nodes;
	delete []status;
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
				if (haystack[m_start + i] == needle)
					n++;

				haystack[m_start + i] = n;
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

	static elem_t needle;
	static elem_t *haystack;

private:
	size_t m_start;
	size_t m_len;
	long *m_cnt;
};

elem_t CounterTask::needle;
elem_t *CounterTask::haystack;


int main(int argc, char *argv[])
{
	size_t logn = 8;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		logn = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();



	size_t n = 1 << logn;
	CounterTask::haystack = new elem_t[n];
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);

	unsigned ss = sizeof(elem_t) * n / 2;

    hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 0);

	hwloc_set_area_membind(topology, CounterTask::haystack, ss, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);

    obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 1);
	hwloc_set_area_membind(topology, CounterTask::haystack + n/2, ss, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);

	cout << "Haystack:     " << n*sizeof(unsigned) / (1024*1024) << " Mb\n";

	uint32_t mask = (1 << 16) - 1;

	std::cerr << "init\n";
	for (size_t i = 0; i < n; ++i)
		CounterTask::haystack[i] = xorshift_rand() & mask;

	CounterTask::needle = xorshift_rand() & mask;

	std::cerr << "moving\n";

	// migrate_pages(CounterTask::haystack, sizeof(unsigned) * n/2, 0);

	long answer = 0;
	auto start = system_clock::now();

	{
		CounterTask root(0, n, &answer);
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
