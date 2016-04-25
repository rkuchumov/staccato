#ifndef TBB_MERGESORT_H
#define TBB_MERGESORT_H

#include <cstring>
#include <algorithm>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../taskexec.h"

template <typename elem_t>
class TbbMergeSort: public tbb::task
{
public:
	TbbMergeSort (elem_t *array_, elem_t *tmp_, size_t left_, size_t right_):
		array(array_), tmp(tmp_), left(left_), right(right_)
	{ };
	~TbbMergeSort () {};

	task *execute() {
		if (right - left <= 100) {
			std::sort(array + left, array + right);
			return NULL;
		}

		size_t mid = (left + right) / 2;
		size_t l = left;
		size_t r = mid;

        set_ref_count(3);

		TbbMergeSort &left_task = *new(allocate_child()) TbbMergeSort(array, tmp, left, mid);
		TbbMergeSort &right_task = *new(allocate_child()) TbbMergeSort(array, tmp, mid, right);
		
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

		return NULL;
	}

private:
	elem_t *array;
	elem_t *tmp;

	size_t left;
	size_t right;
};

class TbbMergeSortExec: public TaskExec
{
public:
	TbbMergeSortExec(size_t log_size, bool is_exp_, 
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

		scheduler_init = new tbb::task_scheduler_init(nthreads);
	}

	~TbbMergeSortExec() {
		delete scheduler_init;

		delete[] array;
		delete[] tmp;
	}

	const char *test_name() {
		if (is_exp)
			return "Tbb_MergesortExp";
		else
			return "Tbb_Mergesort";
	}

	void run() {
		TbbMergeSort<elem_t> &root =
			*new(tbb::task::allocate_root())TbbMergeSort <elem_t>(array, tmp, 0, size);
		tbb::task::spawn_root_and_wait(root);
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

private:
	tbb::task_scheduler_init *scheduler_init; 

	typedef int elem_t;

	bool is_exp;
	size_t size;
	elem_t sum_before;
	elem_t *tmp;
	elem_t *array;
};

#endif /* end of include guard: TBB_MERGESORT_H */

