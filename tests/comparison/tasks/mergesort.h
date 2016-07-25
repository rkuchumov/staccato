#ifndef MERGESORT_H
#define MERGESORT_H

#include <cstring>

#include "../test_suite.h"

using namespace std;

template<typename task, typename elem_t, bool no_del = false>
class mergesort_task: public task
{
public:
	mergesort_task(elem_t *array_, elem_t *tmp_, size_t left_, size_t right_):
		array(array_), tmp(tmp_), left(left_), right(right_)
	{ };

	~mergesort_task () {};

	void my_execute() {
		if (right - left <= 1) {
			return;
		}

		size_t mid = (left + right) / 2;
		size_t l = left;
		size_t r = mid;

		vector<task *> task_list(2);
		task_list[0] = new(this) mergesort_task<task, elem_t, no_del>(array, tmp, left, mid);
		task_list[1] = new(this) mergesort_task<task, elem_t, no_del>(array, tmp, mid, right);
		
		task::exec_and_wait(task_list);

		if (!no_del) {
			delete task_list[0];
			delete task_list[1];
		}

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

template<typename scheduler, typename task, bool no_del = false>
class mergesort_test: public abstract_test
{
public:
	mergesort_test(string name_, int log_size = 24, bool is_exp_ = false, size_t nthreads = 0, size_t k = 0) 
		: abstract_test(name_), is_exp(is_exp_)
	{
		size = 1 << log_size;

		array = new elem_t[size];
		tmp = new elem_t[size];

		sh = new scheduler(nthreads, k);
	}

	~mergesort_test() {
		delete sh;

		delete[] array;
		delete[] tmp;
	}

	unsigned long get_steals() {
		return sh->get_steals();
	}

	double get_delay() {
		return sh->get_delay();
	}

	void set_up() {
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

		root = new mergesort_task<task, elem_t, no_del> (array, tmp, 0, size);
	}

	void iteration() {
		sh->runRoot(root);
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

	void break_down() {
		if (!no_del)
			delete root;
	}

private:
	typedef int elem_t;

	bool is_exp;
	size_t size;
	elem_t sum_before;
	elem_t *tmp;
	elem_t *array;

	scheduler *sh;

	mergesort_task<task, elem_t, no_del> *root;
};

#endif /* end of include guard: MERGESORT_H */

