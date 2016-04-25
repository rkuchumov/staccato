#ifndef MERGESORT_H
#define MERGESORT_H

#include <cstring>
#include <algorithm>

#include "taskexec.h"

#include "scheduler.h"

template <typename elem_t>
class MergeSort: public staccato::task
{
public:
	MergeSort (elem_t *array_, elem_t *tmp_, size_t left_, size_t right_):
		array(array_), tmp(tmp_), left(left_), right(right_)
	{ };
	~MergeSort () {};

	void execute() {
		if (right - left <= 100) {
			std::sort(array + left, array + right);
			return;
		}

		size_t mid = (left + right) / 2;
		size_t l = left;
		size_t r = mid;

		MergeSort *left_task = new MergeSort(array, tmp, left, mid);
		MergeSort *right_task = new MergeSort(array, tmp, mid, right);
		
		spawn(left_task);
		spawn(right_task);

		wait_for_all();

		for (size_t i = left; i < right; i++) {
			if ((l < mid && r < right && array[l] < array[r]) || r == right) {
				tmp[i] = array[l];
				l++;
			} else if ((l < mid && r < right) || l == mid) {
				tmp[i] = array[r];
				r++;
			}
		}

		memcpy(array + left, tmp + left, sizeof(elem_t) * (right - left));
	}

private:
	elem_t *array;
	elem_t *tmp;

	size_t left;
	size_t right;
};

class MergeSortExec: public TaskExec
{
public:
	MergeSortExec(size_t log_size, bool is_exp_, 
			size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size), is_exp(is_exp_)
	{
		size = 1 << log_size;

		array = new elem_t[size];
		tmp = new elem_t[size];

		sum_before = 0;
		for (size_t i = 0; i < size; i++) {
			if (is_exp) {
				double r = static_cast<double> (rand()) / RAND_MAX;
				array[i] = static_cast<elem_t> (-log(r) * size);
			} else {
				array[i] = rand() % size;
			}

			sum_before += array[i];
		}

		if (steal_size > 1)
			staccato::scheduler::deque_log_size = 11;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	~MergeSortExec() {
		staccato::scheduler::terminate();

		delete[] array;
		delete[] tmp;
	}

	const char *test_name() {
		if (is_exp)
			return "MergesortExp";
		else
			return "Mergesort";
	}

	void run() {
		MergeSort<elem_t> root(array, tmp, 0, size);
		root.execute();
	}

	bool check() {
		elem_t sum_after = array[0];
		for (size_t i = 1; i < size; i++) {
			if (array[i] < array[i - 1])
				return false;
			sum_after += array[i];
		}

		return sum_before == sum_after;
	}

	static void init_scheduler(size_t nthreads = 0, size_t steal_size = 1, size_t deque_log_size = 11) {
		if (steal_size > 1)
			staccato::scheduler::deque_log_size = deque_log_size;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	static void terminate_scheduler() {
		staccato::scheduler::terminate();
	}

private:
	typedef int elem_t;

	bool is_exp;
	size_t size;
	elem_t sum_before;
	elem_t *tmp;
	elem_t *array;
};

#endif /* end of include guard: MERGESORT_H */

