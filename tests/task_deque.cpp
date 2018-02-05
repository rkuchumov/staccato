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
	auto d = new task_deque<task_mock>(8);
	delete d;
}

TEST(take, single) {
	size_t ntasks = 8;
	auto set = new task_deque<task_mock>(ntasks);

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
	}


	for (size_t i = ntasks; i >= 1; --i) {
		auto r = set->take();
		auto t = reinterpret_cast<task_mock *>(r);

		EXPECT_EQ(t->id, i);
	}

	// EXPECT_TRUE(set->empty());

	delete set;
}

TEST(steal, single) {
	size_t ntasks = 8;
	auto set = new task_deque<task_mock>(ntasks);

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
	}


	for (size_t i = 1; i <= ntasks; ++i) {
		bool was_empty = false;
		auto r = set->steal(&was_empty);
		auto t = reinterpret_cast<task_mock *>(r);

		EXPECT_FALSE(was_empty);
		EXPECT_EQ(t->id, i);
	}

	bool was_empty = false;
	auto r = set->steal(&was_empty);
	EXPECT_EQ(r, nullptr);
	EXPECT_TRUE(was_empty);

	// EXPECT_TRUE(set->empty());

	delete set;
}

TEST(steal, concurrent_steals) {
	size_t ntasks = 1 << 13;

	std::vector<size_t> tasks;

	auto set = new task_deque<task_mock>(ntasks);

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
		tasks.push_back(i);
	}

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<size_t > stolen[nthreads];
	std::thread threads[nthreads];

	auto steal = [&](size_t id) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			bool was_empty = false;
			auto r = set->steal(&was_empty);
			if (r) {
				auto t = reinterpret_cast<task_mock *>(r);
				stolen[id].push_back(t->id);
			}
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
		}
	}

	EXPECT_TRUE(tasks.empty());

	delete set;
}

TEST(steal_take, concurrent_steal_and_take) {
	size_t ntasks = 1 << 13;

	auto set = new task_deque<task_mock>(ntasks);

	std::vector<size_t> tasks;

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
		tasks.push_back(i);
	}

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<size_t> stolen[nthreads];
	std::vector<size_t> taken;
	std::thread threads[nthreads];

	auto steal = [&](size_t id, bool steal) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			bool was_empty = false;
			auto r = steal ? set->steal(&was_empty) : set->take();
			if (r) {
				auto t = reinterpret_cast<task_mock *>(r);
				stolen[id].push_back(t->id);
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
		}
	}

	// EXPECT_TRUE(tasks.empty());

	delete set;
}

