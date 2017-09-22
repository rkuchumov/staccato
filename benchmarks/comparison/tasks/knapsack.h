#ifndef KNAPSACK_H
#define KNAPSACK_H

#include <mutex>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "../test_suite.h"

using namespace std;

template<typename task, bool no_del = false>
class knapsack_task: public task
{
public:
	knapsack_task (bool *x_ = NULL, long cost_ = 0, long weight_ = 0, size_t depth_ = 0):
		x(x_), cost(cost_), weight(weight_), depth(depth_) {};
	~knapsack_task () {};

	void my_execute() {
		if (depth == N) {
			record_lock.lock();

			if (cost > cost_max) {
				cost_max = cost;
				memcpy(record, x, N);
			}

			record_lock.unlock();

			return;
		}

		bool *take = NULL;
		bool *not_take = NULL;

		vector<task *> task_list;

		if (weight + w[depth] <= max_weight) {
			take = new bool[N]();
			if (depth > 0)
				memcpy(take, x, depth);
			take[depth] = true;

			knapsack_task *take_task =
				new(this) knapsack_task(take, cost + p[depth], weight + w[depth], depth + 1);
			task_list.push_back(take_task);
		}

		not_take = new bool[N]();
		if (depth > 0)
			memcpy(not_take, x, depth);

		knapsack_task *no_take_task =
			new(this) knapsack_task(not_take, cost, weight, depth + 1);

		task_list.push_back(no_take_task);

		task::exec_and_wait(task_list);
		delete take;

		if (not_take)
			delete not_take;

		if (!no_del) {
			for (auto *t : task_list)
				delete t;
		}

		return;
	}

	// TODO: names look ugly
	static long max_weight;
	static size_t N;
	static long *p;
	static long *w;

	static std::mutex record_lock;
	static long cost_max;
	static bool *record;

private:
	bool *x;
	long cost;
	long weight;
	size_t depth;
};

template<typename task, bool no_del>
long knapsack_task<task, no_del>::max_weight;

template<typename task, bool no_del>
size_t knapsack_task<task, no_del>::N;

template<typename task, bool no_del>
long *knapsack_task<task, no_del>::p;

template<typename task, bool no_del>
long *knapsack_task<task, no_del>::w;

template<typename task, bool no_del>
std::mutex knapsack_task<task, no_del>::record_lock;

template<typename task, bool no_del>
long knapsack_task<task, no_del>::cost_max;

template<typename task, bool no_del>
bool *knapsack_task<task, no_del>::record;


template<typename scheduler, typename task, bool no_del = false>
class knacksack_test: public abstract_test
{
public:
	knacksack_test(string name_, unsigned N = 24, size_t nthreads = 0, size_t k = 0) 
		: abstract_test(name_)
	{
		knapsack_task<task, no_del>::N = N;
		knapsack_task<task, no_del>::p = new long[N]();
		knapsack_task<task, no_del>::w = new long[N]();
		knapsack_task<task, no_del>::record = new bool[N]();

		knapsack_task<task, no_del>::max_weight = 100 * N;
		for (size_t i = 0; i < N; i++) {
			knapsack_task<task, no_del>::p[i] = my_rand() % knapsack_task<task, no_del>::max_weight;
			knapsack_task<task, no_del>::w[i] = my_rand() % 600;
		}

		sh = new scheduler(nthreads, k);
	}

	~knacksack_test() {
		delete sh;
	}

	unsigned long get_steals() {
		return sh->get_steals();
	}

	double get_delay() {
		return sh->get_delay();
	}

	void set_up() {
		root = new knapsack_task<task, no_del> ();
	}

	void iteration() {
		sh->runRoot(root);
	}

	void break_down() {
		// ans = 0;
		if (!no_del)
			delete root;
	}


private:
	scheduler *sh;
	knapsack_task<task, no_del> *root;

	inline unsigned long my_rand() {
		static unsigned long x = 123456789;
		static unsigned long y = 362436069;
		static unsigned long z = 521288629;

		x ^= x << 16;
		x ^= x >> 5;
		x ^= x << 1;

		unsigned long t = x;
		x = y;
		y = z;
		z = t ^ x ^ y;

		return z;
	}

};

#endif /* end of include guard: KNAPSACK_H */
