/*
 * Blocked matrix multiply is done as follows:
 * Adapted from Cilk 5.4.3 example
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstring>

#include <cilk/cilk.h>
#include <cilk/cilk_api.h> 

using namespace std;
using namespace chrono;

inline uint32_t xorshift_rand() {
	static uint32_t x = 2463534242;
	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

static const uint32_t rand_max = 1e3;

typedef uint32_t elem_t;

class Block
{
public:
	Block();

	void fill();

	void add(Block *a, Block *b); 

	void mul(Block *a, Block *b, bool add = false);

	elem_t trace();

	static elem_t dotpord(Block *a, Block *b);

	static const size_t size = 256;

private:
	elem_t m_data[size];
};

Block::Block()
{
	memset(m_data, 0, size);
}

void Block::fill()
{
	for (size_t i = 0; i < size; i += 4) {
		m_data[i + 0] += xorshift_rand() % rand_max;
		m_data[i + 1] += xorshift_rand() % rand_max;
		m_data[i + 2] += xorshift_rand() % rand_max;
		m_data[i + 3] += xorshift_rand() % rand_max;
	}
}

void Block::add(Block *a, Block *b)
{
	for (size_t i = 0; i < size; i += 4) {
		m_data[i + 0] = a->m_data[i + 0] + b->m_data[i + 0];
		m_data[i + 1] = a->m_data[i + 1] + b->m_data[i + 1];
		m_data[i + 2] = a->m_data[i + 2] + b->m_data[i + 2];
		m_data[i + 3] = a->m_data[i + 3] + b->m_data[i + 3];
	}
}

void Block::mul(Block *a, Block *b, bool add)
{
	for (size_t j = 0; j < 16; j += 2) {
		elem_t *bp = &b->m_data[j];
		for (size_t i = 0; i < 16; i += 2) {
			elem_t *ap = &a->m_data [i * 16];
			elem_t *rp = &m_data[i * 16 + j];

			elem_t s0_0 = ap[0]   * bp[0];
			elem_t s0_1 = ap[0]   * bp[1];
			elem_t s1_0 = ap[16]  * bp[0];
			elem_t s1_1 = ap[16]  * bp[1];
			s0_0 += ap[1]  * bp[16];
			s0_1 += ap[1]  * bp[17];
			s1_0 += ap[17] * bp[16];
			s1_1 += ap[17] * bp[17];
			s0_0 += ap[2]  * bp[32];
			s0_1 += ap[2]  * bp[33];
			s1_0 += ap[18] * bp[32];
			s1_1 += ap[18] * bp[33];
			s0_0 += ap[3]  * bp[48];
			s0_1 += ap[3]  * bp[49];
			s1_0 += ap[19] * bp[48];
			s1_1 += ap[19] * bp[49];
			s0_0 += ap[4]  * bp[64];
			s0_1 += ap[4]  * bp[65];
			s1_0 += ap[20] * bp[64];
			s1_1 += ap[20] * bp[65];
			s0_0 += ap[5]  * bp[80];
			s0_1 += ap[5]  * bp[81];
			s1_0 += ap[21] * bp[80];
			s1_1 += ap[21] * bp[81];
			s0_0 += ap[6]  * bp[96];
			s0_1 += ap[6]  * bp[97];
			s1_0 += ap[22] * bp[96];
			s1_1 += ap[22] * bp[97];
			s0_0 += ap[7]  * bp[112];
			s0_1 += ap[7]  * bp[113];
			s1_0 += ap[23] * bp[112];
			s1_1 += ap[23] * bp[113];
			s0_0 += ap[8]  * bp[128];
			s0_1 += ap[8]  * bp[129];
			s1_0 += ap[24] * bp[128];
			s1_1 += ap[24] * bp[129];
			s0_0 += ap[9]  * bp[144];
			s0_1 += ap[9]  * bp[145];
			s1_0 += ap[25] * bp[144];
			s1_1 += ap[25] * bp[145];
			s0_0 += ap[10] * bp[160];
			s0_1 += ap[10] * bp[161];
			s1_0 += ap[26] * bp[160];
			s1_1 += ap[26] * bp[161];
			s0_0 += ap[11] * bp[176];
			s0_1 += ap[11] * bp[177];
			s1_0 += ap[27] * bp[176];
			s1_1 += ap[27] * bp[177];
			s0_0 += ap[12] * bp[192];
			s0_1 += ap[12] * bp[193];
			s1_0 += ap[28] * bp[192];
			s1_1 += ap[28] * bp[193];
			s0_0 += ap[13] * bp[208];
			s0_1 += ap[13] * bp[209];
			s1_0 += ap[29] * bp[208];
			s1_1 += ap[29] * bp[209];
			s0_0 += ap[14] * bp[224];
			s0_1 += ap[14] * bp[225];
			s1_0 += ap[30] * bp[224];
			s1_1 += ap[30] * bp[225];
			s0_0 += ap[15] * bp[240];
			s0_1 += ap[15] * bp[241];
			s1_0 += ap[31] * bp[240];
			s1_1 += ap[31] * bp[241];

			if (add) {
				rp[0]  += s0_0;
				rp[1]  += s0_1;
				rp[16] += s1_0;
				rp[17] += s1_1;
			} else {
				rp[0]  = s0_0;
				rp[1]  = s0_1;
				rp[16] = s1_0;
				rp[17] = s1_1;
			}
		}
	}
}

elem_t Block::trace()
{
	elem_t s = 0;
	for (size_t i = 0; i < 16; ++i)
		s += m_data[i * 16 + i];
	return s;
}

// r = sum(transpose(A) o B)
elem_t Block::dotpord(Block *a, Block *b)
{
	elem_t s = 0;
	for (size_t i = 0; i < 16; ++i)
		for (size_t j = 0; j < 16; ++j)
			s += a->m_data[16*j + i] * b->m_data[16*i + j];

	return s;
}

void add(Block *A, Block *B, Block *R, size_t n);
void mul(Block *A, Block *B, Block *R, size_t n);

void operationtask(
	Block *A,
	Block *B,
	Block *R,
	size_t n,
	bool do_mul = true
) {
	if (do_mul)
		mul(A, B, R, n);
	else
		add(A, B, R, n);
}

void add(Block *A, Block *B, Block *R, size_t n)
{
	if (n <= 1) {
		R->add(A, B);
		return;
	}

	auto q = n / 4;

	cilk_spawn operationtask(A+0*q, B+0*q, R+0*q, q, false);
	cilk_spawn operationtask(A+1*q, B+1*q, R+1*q, q, false);
	cilk_spawn operationtask(A+2*q, B+2*q, R+2*q, q, false);
	cilk_spawn operationtask(A+3*q, B+3*q, R+3*q, q, false);

	cilk_sync;
}

/*      A     x     B     =         R         =     l       +     r    
 *  | 0 | 1 |   | 0 | 1 |   | 00+12 | 01+13 |   | 00 | 01 |   | 12 | 13 |
 *  |---+---| x |---+---| = |-------+-------| = |----+----| + |----+----|
 *  | 2 | 3 |   | 2 | 3 |   | 20+32 | 21+33 |   | 20 | 21 |   | 32 | 33 |
 */
void mul(Block *A, Block *B, Block *R, size_t n) {
	if (n <= 1) {
		R->mul(A, B);
		return;
	}

	auto q = n / 4;

	auto l = new Block[n];

	cilk_spawn operationtask(A+0*q, B+0*q, l+0*q, q);
	cilk_spawn operationtask(A+0*q, B+1*q, l+1*q, q);
	cilk_spawn operationtask(A+2*q, B+0*q, l+2*q, q);
	cilk_spawn operationtask(A+2*q, B+1*q, l+3*q, q);

	auto r = new Block[n];

	cilk_spawn operationtask(A+1*q, B+2*q, r+0*q, q);
	cilk_spawn operationtask(A+1*q, B+3*q, r+1*q, q);
	cilk_spawn operationtask(A+3*q, B+2*q, r+2*q, q);
	cilk_spawn operationtask(A+3*q, B+3*q, r+3*q, q);

	cilk_sync;

	A = l;
	B = r;
	add(A, B, R, n);

	delete []r;
	delete []l;
}


void fill(Block *A, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		A[i].fill();
}

elem_t trace(Block *A, size_t n)
{
	if (n <= 1)
		return A->trace();

	auto q = n / 4;
	return trace(A, q) + trace(A + 3*q, q);
}

elem_t dotpord(Block *A, Block *B, size_t n)
{
	if (n <= 1)
		return Block::dotpord(A, B);

	auto q = n / 4;
	elem_t s = 0;
	s += dotpord(A+0*q, B+0*q, q);
	s += dotpord(A+2*q, B+1*q, q);
	s += dotpord(A+1*q, B+2*q, q);
	s += dotpord(A+3*q, B+3*q, q);

	return s;
}

bool check(Block *A, Block *B, Block *C, size_t n)
{
	return fabs(trace(C, n) - dotpord(A, B, n)) < 1e-3;
}

void test(Block *A, Block *B, Block *C, size_t n)
{
	cilk_spawn operationtask(A, B, C, n);
	cilk_sync;
}

int main(int argc, char *argv[])
{
	size_t log_n = 4;
	const char *nthreads = nullptr;

	if (argc >= 2)
		nthreads = argv[1];
	if (argc >= 3)
		log_n = atoi(argv[2]);
	if (nthreads == nullptr)
		nthreads = to_string(thread::hardware_concurrency()).c_str();

	auto n = 1 << log_n;

	cout << "Matrix dim: " << n * 16 << "\n";
	auto nblocks = n * n;

	cout << "Data size:  " << 3 * nblocks * sizeof(Block) / 1024 << "Kb\n";

	auto A = new Block[nblocks];
	auto B = new Block[nblocks];
	auto R = new Block[nblocks];

	fill(A, nblocks);
	fill(B, nblocks);

	__cilkrts_end_cilk(); 

	auto start = system_clock::now();

	if (__cilkrts_set_param("nworkers", nthreads) != 0) {
		cerr << "Failed to set worker count\n";
		exit(EXIT_FAILURE);
	}

	__cilkrts_init();

	test(A, B, R, nblocks);

	__cilkrts_end_cilk(); 

	auto stop = system_clock::now();

	cout << "Scheduler:  cilk\n";
	cout << "Benchmark:  blkmul\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << log_n << "\n";
	cout << "Output:     " << check(A, B, R, nblocks) << "\n";

	delete []A;
	delete []B;
	delete []R;
	return 0;
}

