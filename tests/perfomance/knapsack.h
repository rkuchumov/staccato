#ifndef KNAPSACK_H
#define KNAPSACK_H

#include <mutex>
#include <cstring>
#include <iostream>

#include "taskexec.h"

#include "scheduler.h"

class KnapsackTask: public staccato::task
{
public:
	KnapsackTask (bool *x_ = NULL, long cost_ = 0, long weight_ = 0, size_t depth_ = 0):
		x(x_), cost(cost_), weight(weight_), depth(depth_) {};
	~KnapsackTask () {};

	void execute() {
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

		if (weight + w[depth] <= max_weight) {
			take = new bool[N]();
			if (depth > 0)
				memcpy(take, x, depth);
			take[depth] = true;

			KnapsackTask *take_task =
				new KnapsackTask(take, cost + p[depth], weight + w[depth], depth + 1);
			spawn(take_task);
		}

		not_take = new bool[N]();
		if (depth > 0)
			memcpy(not_take, x, depth);

		KnapsackTask *no_take_task =
			new KnapsackTask(not_take, cost, weight, depth + 1);
		spawn(no_take_task);

		wait_for_all();
		delete take;

		if (not_take)
			delete not_take;

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

long KnapsackTask::max_weight;
size_t KnapsackTask::N;
long *KnapsackTask::p;
long *KnapsackTask::w;

std::mutex KnapsackTask::record_lock;
long KnapsackTask::cost_max;
bool *KnapsackTask::record;

class KnapsackTaskExec: public TaskExec
{
public:
	KnapsackTaskExec(unsigned N, size_t nthreads, size_t iterations, size_t steal_size):
		TaskExec(nthreads, iterations, steal_size)
	{
		KnapsackTask::N = N;
		KnapsackTask::p = new long[N]();
		KnapsackTask::w = new long[N]();
		KnapsackTask::record = new bool[N]();

		KnapsackTask::max_weight = 100 * N;
		for (size_t i = 0; i < N; i++) {
			KnapsackTask::p[i] = my_rand() % KnapsackTask::max_weight;
			KnapsackTask::w[i] = my_rand() % 600;
		}

		if (steal_size > 1)
			staccato::scheduler::deque_log_size = 11;
		staccato::scheduler::tasks_per_steal = steal_size;

		staccato::scheduler::initialize(nthreads);
	}

	~KnapsackTaskExec() {
		staccato::scheduler::terminate();
	}

	const char *test_name() {
		return "Knapsack";
	}

	void run() {
		KnapsackTask root;
		root.execute();
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

};

#endif /* end of include guard: KNAPSACK_H */
