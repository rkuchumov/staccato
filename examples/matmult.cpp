#include <iostream>
#include <cassert>
#include <chrono>

using namespace std;

#include "scheduler.h"

#define PRINT_MATRICES 0

typedef long elem_t;
elem_t **A;
elem_t **B;

elem_t **allocate_matrix(size_t size)
{
	elem_t **m = new elem_t*[size];
	for (size_t i = 0; i < size; i++)
		m[i] = new elem_t[size];

	return m;
}

void fill_matrix(elem_t **m, size_t size, int s)
{
	for (size_t i = 0; i < size; i++)
		for (size_t j = 0; j < size; j++)
			m[i][j] = s * i + (2*s + 1) * j;
			// m[i][j] = rand() % 40;
}

void print_matrix(elem_t **m, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++)
			cout << m[i][j] << " \t";
		cout << "\n";
	}
}

struct MultTask: public task
{
	size_t col_a, row_a;
	size_t col_b, row_b;
	size_t col_c, row_c;
	size_t size;

	elem_t **out;

	elem_t **left;
	elem_t **right;

	MultTask(
		elem_t **out_, size_t size_,
		size_t col_a_ = 0, size_t row_a_ = 0,
		size_t col_b_ = 0, size_t row_b_ = 0,
		size_t col_c_ = 0, size_t row_c_ = 0)
	{ 
		col_a = col_a_; row_a = row_a_;
		col_b = col_b_; row_b = row_b_;
		col_c = col_c_; row_c = row_c_;
		size = size_; out = out_;

		if (size > 1) {
			left = allocate_matrix(size);
			right = allocate_matrix(size);
		}
	};

	void execute() {
		if (size == 1) {
			out[col_c][row_c] = A[col_a][row_a] * B[col_b][row_b];
			return;
		}

		size_t s = size / 2;

		MultTask *l[2][2];
		MultTask *r[2][2];

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				l[i][j] = new MultTask(left, s, col_a + i*s, row_a, col_b, row_b + j*s, i*s, j*s);
				spawn(l[i][j]);
				r[i][j] = new MultTask(right, s, col_a + i*s, row_a + s, col_b + s, row_b + j*s, i*s, j*s);
				spawn(r[i][j]);
			}
		}

		wait_for_all();

		for (size_t i = 0; i < size; i++) {
			for (size_t j = 0; j < size; j++)
				out[col_c + i][row_c + j] = left[i][j] + right[i][j];

			delete []left[i];
			delete []right[i];
		}

		delete []left;
		delete []right;

		return;
	}
};

int main(int argc, char *argv[])
{
	srand(time(NULL));
	int log_size = 6;
	int iter = 1;

	if (argc == 3) {
		log_size = atoi(argv[1]);
		iter = atoi(argv[2]);

		if (log_size <= 0 || iter <= 0) {
			cout << "Can't parse arguments\n";
			exit(EXIT_FAILURE);
		}
	} else {
		cout << "Usage:\n   " << argv[0];
		cout << " <Log_2 size (" << log_size << ")>";
		cout << " <Iterations (" << iter << ")>\n\n";
	}

	size_t size = 1 << log_size;

	A = allocate_matrix(size);
	fill_matrix(A, size, 1);
	B = allocate_matrix(size);
	fill_matrix(B, size, 2);
	elem_t **C = allocate_matrix(size);

#if PRINT_MATRICES
	cout << "A:\n";
	print_matrix(A, size);
	cout << "B:\n";
	print_matrix(A, size);
#endif

	scheduler::initialize();

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iter; i++) {
		MultTask *root = new MultTask(C, size);
		root->execute();
		delete root;
	}

	auto end = std::chrono::steady_clock::now();

	scheduler::terminate();

#if PRINT_MATRICES
	cout << "C:\n";
	print_matrix(C, size);
#endif

	elem_t trace_AB = 0;
	elem_t trace_C = 0;
	for (size_t i = 0; i < size; i++) {
		for (size_t k = 0; k < size; k++)
			trace_AB += A[i][k] * B[k][i];
		trace_C += C[i][i];
	}

	if (trace_AB == trace_C)
		cout << "tr(A*B) = tr(C) = " << trace_C << "\n";
	else 
		cout << "Multiplication Errorn\n";

	double dt = (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
	cout << "Iteration time: " << dt / iter << " sec\n";
	cout << "Elapsed time: " << dt << " sec\n";

	return 0;
}
