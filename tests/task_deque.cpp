#include <list>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "task_mock.hpp"

#include "task_deque.hpp"

using namespace staccato;
using namespace staccato::internal;

static const size_t test_timeout = 2;
static const size_t nthreads = 4;

TEST(ctor, creating_and_deleteing) {
	auto d = new task_deque(4);
	delete d;
}

TEST(take, without_resize) {
	size_t size = 5;
	size_t ntasks = 1 << (size - 1);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	for (size_t i = 0; i < ntasks; ++i) {
		auto t = new task_mock();
		tasks.push_back(t);
		deque->put(t);
	}

	for (size_t i = 0; i < ntasks; ++i) {
		auto t1 = deque->take();
		auto t2 = tasks.back();
		ASSERT_EQ(t1, t2);
		tasks.pop_back();

		delete t1;
	}

	delete deque;
}

TEST(take, with_resize) {
	size_t size = 5;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	for (size_t i = 0; i < ntasks; ++i) {
		auto t = new task_mock();
		tasks.push_back(t);
		deque->put(t);
	}

	for (size_t i = 0; i < ntasks; ++i) {
		auto t1 = deque->take();
		auto t2 = tasks.back();
		ASSERT_EQ(t1, t2);
		tasks.pop_back();

		delete t1;
	}

	delete deque;
}

TEST(steal, sequential) {
	size_t size = 5;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	for (size_t i = 0; i < ntasks; ++i) {
		auto t = new task_mock();
		tasks.push_back(t);
		deque->put(t);
	}

	std::reverse(tasks.begin(), tasks.end());

	for (size_t i = 0; i < ntasks; ++i) {
		auto t1 = deque->steal();
		auto t2 = tasks.back();
		ASSERT_EQ(t1, t2);
		tasks.pop_back();
		delete t1;
	}

	delete deque;
}

TEST(steal, concurrent_steal) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	for (size_t i = 0; i < ntasks; ++i) {
		auto t = new task_mock();
		tasks.push_back(t);
		deque->put(t);
	}

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<task *> stolen[nthreads];
	std::thread threads[nthreads];

	auto steal = [&](size_t id) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			auto t = deque->steal();
			if (t)
				stolen[id].push_back(t);
		}
	};

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i] = std::thread(steal, i);
	}

	std::this_thread::sleep_for(std::chrono::seconds(test_timeout));
	stop = true;

	std::cerr << "total tasks: " << ntasks << "\n";
	for (size_t i = 0; i < nthreads; ++i) {
		threads[i].join();
		std::cerr << "thread #" << i << " tasks: " << stolen[i].size() << "\n";
	}

	for (size_t i = 0; i < nthreads; ++i) {
		for (auto t : stolen[i]) {
			auto found = std::find(tasks.begin(), tasks.end(), t);
			ASSERT_TRUE(found != tasks.end());
			tasks.erase(found);

			delete t;
		}
	}

	EXPECT_TRUE(tasks.empty());

	delete deque;
}

TEST(steal_take, concurrent_steal_and_take) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	for (size_t i = 0; i < ntasks; ++i) {
		auto t = new task_mock();
		tasks.push_back(t);
		deque->put(t);
	}

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<task *> stolen[nthreads];
	std::vector<task *> taken;
	std::thread threads[nthreads];

	auto steal = [&](size_t id, bool steal) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			auto t = steal ? deque->steal() : deque->take();
			if (t)
				stolen[id].push_back(t);
		}
	};

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i] = std::thread(steal, i, i != 0);
	}

	std::this_thread::sleep_for(std::chrono::seconds(test_timeout));
	stop = true;

	std::cerr << "total tasks: " << ntasks << "\n";
	for (size_t i = 0; i < nthreads; ++i) {
		threads[i].join();
		std::cerr << "thread #" << i << " tasks: " << stolen[i].size() << "\n";
	}

	for (size_t i = 0; i < nthreads; ++i) {
		for (auto t : stolen[i]) {
			auto found = std::find(tasks.begin(), tasks.end(), t);
			ASSERT_TRUE(found != tasks.end());
			tasks.erase(found);

			delete t;
		}
	}

	EXPECT_TRUE(tasks.empty());

	delete deque;
}


TEST(put_steal_take, concurrent_steal_take_and_put) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size);

	std::vector<task *> tasks;

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::atomic_size_t tasks_left(ntasks);

	std::vector<task *> stolen[nthreads];
	std::vector<task *> taken;
	std::thread threads[nthreads];

	auto steal = [&](size_t id, bool steal) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			if (steal) {
				auto t = deque->steal();
				if (t)
					stolen[id].push_back(t);
			} else {
				if (tasks_left > 0) {
					auto new_task = new task_mock();
					tasks.push_back(new_task);
					deque->put(new_task);
					tasks_left--;
				}

				auto t = deque->take();
				if (t)
					stolen[id].push_back(t);
			}
		}
	};

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i] = std::thread(steal, i, i != 0);
	}

	std::this_thread::sleep_for(std::chrono::seconds(test_timeout));
	stop = true;

	std::cerr << "total tasks: " << ntasks << "\n";
	for (size_t i = 0; i < nthreads; ++i) {
		threads[i].join();
		std::cerr << "thread #" << i << " tasks: " << stolen[i].size() << "\n";
	}

	for (size_t i = 0; i < nthreads; ++i) {
		for (auto t : stolen[i]) {
			auto found = std::find(tasks.begin(), tasks.end(), t);
			ASSERT_TRUE(found != tasks.end());
			tasks.erase(found);

			delete t;
		}
	}

	EXPECT_TRUE(tasks.empty());

	delete deque;
}
