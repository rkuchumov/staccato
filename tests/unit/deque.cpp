#include <iostream>
#include <cassert>
#include <atomic>
#include <unistd.h>

using namespace std;

#include "scheduler.h"
#include "task.h"
#include "deque.h"

#define TEST(name, func) \
do { \
	std::cout << "\nTest " << __COUNTER__ << ":\t " << name << "\n"; \
	if ((func))  \
		std::cout << "   Passed\n"; \
	else { \
		std::cout << "   Failed\n"; \
		exit(EXIT_FAILURE); \
	} \
} while (false)

struct TestTask: public task
{
	long n;

	TestTask(int n_): n(n_) {};

	void execute() {
		return;
	}
};

void nstealing_worker(task_deque *pool, atomic_bool *start, atomic_bool *stop,
		long *sum, size_t *nstolen)
{
	*sum = 0;
	*nstolen = 0;
	size_t stolen = 0;
	while (!start->load()) {};

	while (!stop->load()) {
		TestTask **t = (TestTask **) (pool->steal_n(&stolen));

		if (t) {
			for (size_t i = 0; i < stolen; i++) {
				*sum += t[i]->n;
				(*nstolen)++;
			}
			delete t;
		}
		for (int i = 0; i < 1000; i++) { }
	}
}

void stealing_worker(task_deque *pool, atomic_bool *start, atomic_bool *stop,
		long *sum, size_t *nstolen)
{
	*sum = 0;
	*nstolen = 0;
	while (!start->load()) {};

	while (!stop->load()) {
		TestTask *t = dynamic_cast<TestTask *> (pool->steal());
		if (t) {
			*sum += t->n;
			(*nstolen)++;
			delete t;
		}
		for (int i = 0; i < 1000; i++) { }
	}
}

bool steals(size_t log_size, size_t exec_time)
{
	size_t size = 1 << log_size;
	task_deque *pool = new task_deque(log_size);

	long sum = 0;
	for (size_t i = 1; i <= size; i++) {
		task *t = new TestTask(i);
		pool->put(t);
		sum += i;
	}

	long sum_1, sum_2;
	size_t nstolen_1, nstolen_2;

	atomic_bool start(false);
	atomic_bool stop(false);

	thread t1(nstealing_worker, pool, &start, &stop, &sum_1, &nstolen_1);
	thread t2(nstealing_worker, pool, &start, &stop, &sum_2, &nstolen_2);

	start = true;
	sleep(exec_time);
	stop = true;

	t1.join();
	t2.join();

	return (size == nstolen_1 + nstolen_2 && sum == sum_1 + sum_2);
}

bool put_and_steals(size_t log_size, size_t exec_time, bool resize)
{
	size_t size = 1 << log_size;
	task_deque *pool = new task_deque( (resize) ? 1 : log_size );

	long sum = 0;
	long sum_1, sum_2;
	size_t nstolen_1, nstolen_2;

	atomic_bool start(false);
	atomic_bool stop(false);

	thread t1(nstealing_worker, pool, &start, &stop, &sum_1, &nstolen_1);
	thread t2(nstealing_worker, pool, &start, &stop, &sum_2, &nstolen_2);

	start = true;
	for (size_t i = 1; i <= size; i++) {
		task *t = new TestTask(i);
		pool->put(t);
		sum += i;
	}
	sleep(exec_time);
	stop = true;

	t1.join();
	t2.join();

	return (size == nstolen_1 + nstolen_2 && sum == sum_1 + sum_2);
}

bool taske_put_steals(size_t log_size, size_t exec_time, bool resize)
{
	size_t size = 1 << log_size;
	task_deque *pool = new task_deque( (resize) ? 1 : log_size );

	long sum = 0;
	long sum_0 = 0, sum_1, sum_2;
	size_t ntaken = 0, nstolen_1, nstolen_2;

	atomic_bool start(false);
	atomic_bool stop(false);

	thread t1(nstealing_worker, pool, &start, &stop, &sum_1, &nstolen_1);
	thread t2(nstealing_worker, pool, &start, &stop, &sum_2, &nstolen_2);

	start = true;
	for (size_t i = 1; i <= size;) {
		TestTask *t = dynamic_cast<TestTask *> (pool->take());
		if (t) {
			sum_0 += t->n;
			ntaken++;
			delete t;
		} else {
			task *t = new TestTask(i);
			pool->put(t);
			sum += i;
			i++;
		}
	}
	sleep(exec_time);
	stop = true;

	t1.join();
	t2.join();

	return (size == ntaken + nstolen_1 + nstolen_2 && sum == sum_0 + sum_1 + sum_2);
}

bool take_put_steals(size_t log_size, size_t exec_time)
{
	size_t size = 1 << log_size;
	task_deque *pool = new task_deque(1);

	long sum = 0;
	long sum_0 = 0, sum_1, sum_2;
	size_t ntaken = 0, nstolen_1, nstolen_2;

	atomic_bool start(false);
	atomic_bool stop(false);

	thread t1(nstealing_worker, pool, &start, &stop, &sum_1, &nstolen_1);
	thread t2(nstealing_worker, pool, &start, &stop, &sum_2, &nstolen_2);

	start = true;
	for (size_t i = 1; i <= size;) {
		TestTask *t = dynamic_cast<TestTask *> (pool->take());
		if (t) {
			sum_0 += t->n;
			ntaken++;
			delete t;
		} else {
			task *t = new TestTask(i);
			pool->put(t);
			sum += i;
			i++;
		}
	}
	sleep(exec_time);
	stop = true;

	t1.join();
	t2.join();

	return (size == ntaken + nstolen_1 + nstolen_2 && sum == sum_0 + sum_1 + sum_2);
}

int main()
{
	TEST("Concurrent steals (2 tasks)", steals(1, 1));
	TEST("Concurrent steals (2^10 tasks)", steals(10, 1));
	TEST("Concurrent steals (2^20 tasks)", steals(16, 1));
	TEST("Put and concurrent steals, no resize (2^20 tasks)", put_and_steals(16, 1, false));
	TEST("Put and concurrent steals (2^20 tasks)", put_and_steals(16, 1, true));
	TEST("Take, put and concurrent steals (2^20 tasks)", take_put_steals(16, 1));

    return 0;
}
