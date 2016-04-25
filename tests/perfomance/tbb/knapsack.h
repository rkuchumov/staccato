#ifndef TBB_KNAPSACK_H
#define TBB_KNAPSACK_H

#include <mutex>
#include <cstring>
#include <iostream>

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../taskexec.h"

class TbbKnapsackTask: public tbb::task
{
public:
	TbbKnapsackTask (bool *x_ = NULL, long cost_ = 0, long weight_ = 0, size_t depth_ = 0):
		x(x_), cost(cost_), weight(weight_), depth(depth_) {};
	~TbbKnapsackTask () {};

	task *execute() {
		if (depth == N) {
			record_lock.lock();

			if (cost > cost_max) {
				cost_max = cost;
				memcpy(record, x, N);
			}

			record_lock.unlock();

			return NULL;
		}

		bool *take = NULL;
		bool *not_take = NULL;

		if (weight + w[depth] <= max_weight) {
			take = new bool[N]();
			if (depth > 0)
				memcpy(take, x, depth);
			take[depth] = true;

			set_ref_count(3);

			TbbKnapsackTask &take_task =
				*new(allocate_child()) TbbKnapsackTask(take, cost + p[depth], weight + w[depth], depth + 1);
			spawn(take_task);
		} else {
			set_ref_count(2);
		}

		not_take = new bool[N]();
		if (depth > 0)
			memcpy(not_take, x, depth);

		TbbKnapsackTask &no_take_task =
			*new(allocate_child()) TbbKnapsackTask(not_take, cost, weight, depth + 1);
		spawn(no_take_task);

		wait_for_all();
		delete take;

		if (not_take)
			delete not_take;

		return NULL;
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

long TbbKnapsackTask::max_weight;
size_t TbbKnapsackTask::N;
long *TbbKnapsackTask::p;
long *TbbKnapsackTask::w;

std::mutex TbbKnapsackTask::record_lock;
long TbbKnapsackTask::cost_max;
bool *TbbKnapsackTask::record;

class TbbKnapsackTaskExec: public TaskExec
{
public:
	TbbKnapsackTaskExec(unsigned N, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size)
	{
		TbbKnapsackTask::N = N;
		TbbKnapsackTask::p = new long[N]();
		TbbKnapsackTask::w = new long[N]();
		TbbKnapsackTask::record = new bool[N]();

		TbbKnapsackTask::max_weight = 100 * N;
		for (size_t i = 0; i < N; i++) {
			TbbKnapsackTask::p[i] = my_rand() % TbbKnapsackTask::max_weight;
			TbbKnapsackTask::w[i] = my_rand() % 600;
		}

		scheduler_init = new tbb::task_scheduler_init(nthreads);
	}

	~TbbKnapsackTaskExec() {
		delete scheduler_init;
	}

	const char *test_name() {
		return "Tbb_Knapsack";
	}

	void run() {
		TbbKnapsackTask &root = *new(tbb::task::allocate_root()) TbbKnapsackTask();
		tbb::task::spawn_root_and_wait(root);
	}

private:
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

	tbb::task_scheduler_init *scheduler_init; 
};

#endif /* end of include guard: TBB_KNAPSACK_H */
