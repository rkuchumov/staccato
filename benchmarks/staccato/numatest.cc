#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <hwloc.h>

using namespace std;
using namespace chrono;

#include <numaif.h>
#include <unistd.h>

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

typedef double elem_t;

int main(int argc, char *argv[])
{
	size_t logn = 8;
	int numa1 = 1;

	if (argc >= 2) logn = atoi(argv[1]);
	if (argc >= 3) numa1 = atoi(argv[2]);

	size_t n = 1 << logn;

	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);

	elem_t *A = new elem_t[n];
	elem_t *B = new elem_t[n];
	elem_t *C = new elem_t[n];
	elem_t *D = new elem_t[n];

	unsigned qs = sizeof(elem_t) * n / 2;
	hwloc_obj_t n0 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 0);
	hwloc_obj_t n1 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, numa1);

	int rc = 0;
	rc = hwloc_set_area_membind(topology, A, qs, n0->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, A + n/2, qs, n1->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, B, qs, n0->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, B + n/2, qs, n1->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, C, qs, n0->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, C + n/2, qs, n1->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, D, qs, n0->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";
	rc = hwloc_set_area_membind(topology, D + n/2, qs, n1->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
	if (rc) std::cout << "oops\n";

	cout << "Haystack:     " << n*sizeof(elem_t) / (1024*1024) << " Mb\n";

	uint32_t mask = (1 << 16) - 1;

#pragma omp parallel for num_threads(14)
	for (size_t i = 0; i < n; ++i) {
		A[i] = xorshift_rand() & mask;
		B[i] = xorshift_rand() & mask;
		C[i] = xorshift_rand() & mask;
	}

	migrate_pages(A, qs, 0);
	migrate_pages(B, qs, 0);
	migrate_pages(C, qs, 0);
	migrate_pages(D, qs, 0);
	migrate_pages(A+n/2, qs, numa1);
	migrate_pages(B+n/2, qs, numa1);
	migrate_pages(C+n/2, qs, numa1);
	migrate_pages(D+n/2, qs, numa1);

	elem_t needle = xorshift_rand() & mask;

	auto start = system_clock::now();

	long nn = 0;
	for (size_t i = 1; i < n; ++i) {
		D[i] = A[i-1] * B[i] + C[i] * D[i-1];
	}

	auto stop = system_clock::now();

	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << logn << "\n";

	return 0;
}
