#include <list>
#include <string>
#include <thread>
#include <atomic>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "task_mock.hpp"

#include "task_deque.hpp"

using namespace staccato;
using namespace staccato::internal;

TEST(ctor, creating_and_deleteing) {
	auto d = new task_deque(4);
	delete d;
}

TEST(put_take, without_resize) {
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

TEST(put_take, with_resize) {
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

TEST(put_steal, sequential) {
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

TEST(put_steal, multiple_steals) {
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
	static const size_t nthreads = 4;
	std::vector<task *> stolen[nthreads];
	std::thread *threads[nthreads];

	auto steal = [&](size_t id, size_t nwait) {
		nready++;

		while (nready != nwait)
			std::this_thread::yield();

		while (true) {
			auto t = deque->steal();
			if (!t)
				break;

			stolen[id].push_back(t);
		}
	};


	for (size_t i = 0; i < nthreads; ++i) {
		threads[i] = new std::thread(steal, i, nthreads);
	}

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i]->join();
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

TEST(put_steal_take, multiple_steals) {
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
	static const size_t nthreads = 4 + 1;
	std::vector<task *> stolen[nthreads];
	std::vector<task *> taken;
	std::thread *threads[nthreads];

	auto steal = [&](size_t id, size_t nwait, bool steal) {
		nready++;

		while (nready != nwait)
			std::this_thread::yield();

		while (true) {
			auto t = steal ? deque->steal() : deque->take();
			if (!t)
				break;

			stolen[id].push_back(t);
		}
	};

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i] = new std::thread(steal, i, nthreads, i != 0);
	}

	for (size_t i = 0; i < nthreads; ++i) {
		threads[i]->join();
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
