#ifndef MATMULT_H
#define MATMULT_H

#include <vector>
#include <string>

#include "../test_suite.h"

using namespace std;

template<typename task, typename T, bool no_del = false>
class matmul_task: public task
{
public:
	matmul_task(
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

	void my_execute() {
		if (size == 1) {
			out[col_c][row_c] = A[col_a][row_a] * B[col_b][row_b];
			return;
		}

		size_t s = size / 2;

		vector<task *> task_list;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				matmul_task *left_task = new(this) matmul_task(left, s, col_a + i*s, row_a, col_b, row_b + j*s, i*s, j*s);
				task_list.push_back(left_task);

				matmul_task *right_task = new(this) matmul_task(right, s, col_a + i*s, row_a + s, col_b + s, row_b + j*s, i*s, j*s);
				task_list.push_back(right_task);
			}
		}

		task::exec_and_wait(task_list);

		for (size_t i = 0; i < size; i++) {
			for (size_t j = 0; j < size; j++)
				out[col_c + i][row_c + j] = left[i][j] + right[i][j];

			delete []left[i];
			delete []right[i];
		}

		delete []left;
		delete []right;

		if (!no_del) {
			for (auto *t : task_list)
				delete t;
		}

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

template<typename task, typename T, bool no_del>
T **matmul_task<task, T, no_del>::A;
template<typename task, typename T, bool no_del>
T **matmul_task<task, T, no_del>::B;

template<typename scheduler, typename task, bool no_del = false>
class matmul_test: public abstract_test
{
public:
	typedef long T;

	matmul_test(string name_, size_t log_size = 8, size_t nthreads = 0) 
		: abstract_test(name_)
	{
		size = 1 << log_size;

		matmul_task<task, T, no_del>::A = matmul_task<task, T, no_del>::allocate_matrix(size);
		fill_matrix(matmul_task<task, T, no_del>::A, size, 1);
		matmul_task<task, T, no_del>::B = matmul_task<task, T, no_del>::allocate_matrix(size);
		fill_matrix(matmul_task<task, T, no_del>::B, size, 2);

		C = matmul_task<task, T, no_del>::allocate_matrix(size);

		sh = new scheduler(nthreads);
	}

	~matmul_test() {
		delete sh;
	}

	void set_up() {
		root = new matmul_task<task, T, no_del> (C, size);
	}

	void iteration() {
		sh->runRoot(root);
	}

	bool check() {
		T trace_AB = 0;
		T trace_C = 0;

		for (size_t i = 0; i < size; i++) {
			for (size_t k = 0; k < size; k++)
				trace_AB += matmul_task<task, T, no_del>::A[i][k] * matmul_task<task, T, no_del>::B[k][i];
			trace_C += C[i][i];
		}

		return trace_AB == trace_C;
	}

	void break_down() {
		if (!no_del)
			delete root;
	}

	static void fill_matrix(T **m, size_t size, int s)
	{
		for (size_t i = 0; i < size; i++)
			for (size_t j = 0; j < size; j++)
				m[i][j] = s * i + (s + 1) * j;
	}

private:
	scheduler *sh;

	matmul_task<task, T, no_del> *root;

	size_t size;
	T **C;
};

#endif /* end of include guard: MATMULT_H */

