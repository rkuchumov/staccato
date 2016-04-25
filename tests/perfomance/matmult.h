#ifndef MATMULT_H
#define MATMULT_H

#include "taskexec.h"
#include "scheduler.h"

template<typename T>
class MatMultTask: public staccato::task
{
public:
	MatMultTask(
		T **out_, size_t size_,
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

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				MatMultTask *left_task = new MatMultTask(left, s, col_a + i*s, row_a, col_b, row_b + j*s, i*s, j*s);
				spawn(left_task);
				MatMultTask *right_task = new MatMultTask(right, s, col_a + i*s, row_a + s, col_b + s, row_b + j*s, i*s, j*s);
				spawn(right_task);
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

	static T **A;
	static T **B;

	static T **allocate_matrix(size_t size)
	{
		T **m = new T*[size];
		for (size_t i = 0; i < size; i++)
			m[i] = new T[size];

		return m;
	}

private:

	size_t col_a, row_a;
	size_t col_b, row_b;
	size_t col_c, row_c;
	size_t size;

	T **out;

	T **left;
	T **right;
};

template<typename T>
T **MatMultTask<T>::A;
template<typename T>
T **MatMultTask<T>::B;

class MatmultExec: public TaskExec
{
public:
	typedef long T;

	MatmultExec(size_t log_size, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size)
	{
		size = 1 << log_size;

		MatMultTask<T>::A = MatMultTask<T>::allocate_matrix(size);
		fill_matrix(MatMultTask<T>::A, size, 1);
		MatMultTask<T>::B = MatMultTask<T>::allocate_matrix(size);
		fill_matrix(MatMultTask<T>::B, size, 2);

		C = MatMultTask<T>::allocate_matrix(size);

		if (steal_size > 1)
			staccato::scheduler::deque_log_size = 13;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	~MatmultExec() {
		staccato::scheduler::terminate();
	}

	const char *test_name() {
		return "Matmult";
	}

	void run() {
		MatMultTask<T> root(C, size);
		root.execute();
	}

	bool check() {
		T trace_AB = 0;
		T trace_C = 0;

		for (size_t i = 0; i < size; i++) {
			for (size_t k = 0; k < size; k++)
				trace_AB += MatMultTask<T>::A[i][k] * MatMultTask<T>::B[k][i];
			trace_C += C[i][i];
		}

		return trace_AB == trace_C;
	}

	static void fill_matrix(T **m, size_t size, int s)
	{
		for (size_t i = 0; i < size; i++)
			for (size_t j = 0; j < size; j++)
				m[i][j] = s * i + (s + 1) * j;
	}

private:
	size_t size;
	T **C;
};

#endif /* end of include guard: MATMULT_H */

