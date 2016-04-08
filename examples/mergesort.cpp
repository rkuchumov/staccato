#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

#include "scheduler.h"

typedef int elem_t;

class MergeSort: public task
{
public:
	MergeSort (elem_t *array_, elem_t *tmp_, size_t left_, size_t right_):
		array(array_), tmp(tmp_), left(left_), right(right_)
	{ };
	~MergeSort () {};

	void execute() {
		if (right - left <= 100) {
			sort(array + left, array + right);
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

int main(int argc, char *argv[])
{
	size_t log_size = 11;
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
		cout << " <Log2_size (" << log_size << ")>";
		cout << " <Iterations (" << iter << ")>\n\n";
	}

	size_t size = 1 << log_size;

	elem_t *array = new elem_t[size];
	elem_t *tmp = new elem_t[size];
	elem_t sum_before = 0;
	for (size_t i = 0; i < size; i++) {
		array[i] = rand() % size;
		sum_before += array[i];
	}

	scheduler::initialize(4);

	MergeSort *root = new MergeSort(array, tmp, 0, size);

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iter; i++)
		root->execute();

	auto end = std::chrono::steady_clock::now();

	scheduler::terminate();

	elem_t sum_after = array[0];
	for (size_t i = 1; i < size; i++) {
		if (array[i] < array[i - 1]) {
			cout << "Sorting Error (Wrong Order)" << "\n";
			break;
		}
		sum_after += array[i];
	}

	if (sum_before == sum_after)
		cout << "Sorted\n";
	else
		cout << "Sorting Error (Sums are not euqal)" << "\n";

	double dt = (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
	cout << "Iteration time: " << dt / iter << " sec\n";
	cout << "Elapsed time: " << dt << " sec\n";

    return 0;
}
