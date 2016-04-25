#ifndef TBB_MATMULT_H
#define TBB_MATMULT_H

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../taskexec.h"

template<typename T>
class TbbMatMultTask: public tbb::task
{
public:
	TbbMatMultTask(
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

	task *execute() {
		if (size == 1) {
			out[col_c][row_c] = A[col_a][row_a] * B[col_b][row_b];
			return NULL;
		}

		size_t s = size / 2;

        set_ref_count(9);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				TbbMatMultTask &left_task = *new(allocate_child()) TbbMatMultTask(left, s, col_a + i*s, row_a, col_b, row_b + j*s, i*s, j*s);
				spawn(left_task);
				TbbMatMultTask &right_task = *new(allocate_child()) TbbMatMultTask(right, s, col_a + i*s, row_a + s, col_b + s, row_b + j*s, i*s, j*s);
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

		return NULL;
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
T **TbbMatMultTask<T>::A;
template<typename T>
T **TbbMatMultTask<T>::B;

class TbbMatmultExec: public TaskExec
{
public:
	typedef long T;

	TbbMatmultExec(size_t log_size, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size)
	{
		size = 1 << log_size;

		TbbMatMultTask<T>::A = TbbMatMultTask<T>::allocate_matrix(size);
		fill_matrix(TbbMatMultTask<T>::A, size, 1);
		TbbMatMultTask<T>::B = TbbMatMultTask<T>::allocate_matrix(size);
		fill_matrix(TbbMatMultTask<T>::B, size, 2);

		C = TbbMatMultTask<T>::allocate_matrix(size);

		scheduler_init = new tbb::task_scheduler_init(nthreads);
	}

	~TbbMatmultExec() {
		delete scheduler_init;
	}

	const char *test_name() {
		return "Tbb_Matmult";
	}

	void run() {
		TbbMatMultTask<T> &root = *new(tbb::task::allocate_root()) TbbMatMultTask<T>(C, size);
		tbb::task::spawn_root_and_wait(root);
	}

	bool check() {
		T trace_AB = 0;
		T trace_C = 0;

		for (size_t i = 0; i < size; i++) {
			for (size_t k = 0; k < size; k++)
				trace_AB += TbbMatMultTask<T>::A[i][k] * TbbMatMultTask<T>::B[k][i];
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
	tbb::task_scheduler_init *scheduler_init; 

	size_t size;
	T **C;
};

#endif /* end of include guard: TBB_MATMULT_H */

