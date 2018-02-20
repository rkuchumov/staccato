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
	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * 8));
	auto d = new task_deque<task_mock>(3, m);
	delete d;
	free(m);
}

TEST(take, single) {
	size_t ntasks = 8;
	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * ntasks));
	auto set = new task_deque<task_mock>(3, m);

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
	}

	for (size_t i = ntasks; i >= 1; --i) {
		size_t nstolen = 0;
		auto r = set->take(&nstolen);
		auto t = reinterpret_cast<task_mock *>(r);

		EXPECT_EQ(t->id, i);
	}

	EXPECT_TRUE(set->null());

	delete set;
	free(m);
}

TEST(steal, single) {
	size_t ntasks = 8;
	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * ntasks));
	auto set = new task_deque<task_mock>(3, m);

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
	}


	for (size_t i = 1; i <= ntasks; ++i) {
		bool was_null = false;
		bool was_empty = false;
		auto r = set->steal(&was_empty, &was_null);
		auto t = reinterpret_cast<task_mock *>(r);

		EXPECT_FALSE(was_empty);
		EXPECT_EQ(t->id, i);
	}

	bool was_empty = false;
	bool was_null = false;
	auto r = set->steal(&was_empty, &was_null);
	EXPECT_EQ(r, nullptr);
	EXPECT_TRUE(was_empty);

	// EXPECT_TRUE(set->empty());

	delete set;
	free(m);
}

TEST(steal, concurrent_steals) {
	size_t ntasks = 1 << 13;

	std::vector<size_t> tasks;

	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * ntasks));
	auto set = new task_deque<task_mock>(13, m);

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
			bool was_null = false;
			auto r = set->steal(&was_empty, &was_null);
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
	free(m);
}

TEST(steal_take, concurrent_steal_and_take) {
	size_t ntasks = 1 << 13;

	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * ntasks));
	auto set = new task_deque<task_mock>(13, m);

	std::vector<size_t> tasks;

	for (size_t i = 1; i <= ntasks; ++i) {
		new(set->put_allocate()) task_mock(i);
		set->put_commit();
		tasks.push_back(i);
	}

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<size_t> stolen[nthreads];
	std::thread threads[nthreads];

	auto steal = [&](size_t id, bool steal) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			bool was_empty = false;
			bool was_null = false;
			size_t nstolen = 0;

			auto r = steal ? set->steal(&was_empty, &was_null) : set->take(&nstolen);
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
	free(m);
}


TEST(steal_take_put, concurrent_with_reset) {
	size_t iter = 1000;
	size_t ntasks = 8;

	auto m = static_cast<task_mock *>(malloc(sizeof(task_mock) * ntasks));
	auto set = new task_deque<task_mock>(3, m);

	std::vector<size_t> tasks;

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::vector<size_t> taken[nthreads];
	std::thread threads[nthreads];

	auto owner = [&]() {
		nready++;
		while (nready != nthreads)
			std::this_thread::yield();

		for (size_t i = 0; i < iter; ++i) {

			for (size_t j = 0; j < ntasks; ++j) {
				auto t = new(set->put_allocate()) task_mock(i * ntasks + j);
				set->put_commit();
				tasks.push_back(t->id);
			}

			while (true) {
				size_t nstolen = 0;
				auto t = set->take(&nstolen);

				if (t) {
					taken[0].push_back(t->id);
					continue;
				}

				if (nstolen)
					continue;

				break;
			}
		}

		stop = true;
	};

	auto thief = [&](size_t id) {
		nready++;
		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {
			bool was_empty = false;
			bool was_null = false;

			auto t = set->steal(&was_empty, &was_null); 

			if (t) {
				taken[id].push_back(t->id);
				set->return_stolen();
				continue;
			}
		}
	};

	threads[0] = std::thread(owner);
	for (size_t i = 1; i < nthreads; ++i) {
		threads[i] = std::thread(thief, i);
	}

	for (int i = 0; i < 5; ++i) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if (stop)
			break;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	stop = true;

	std::cerr << "total tasks: " << iter * ntasks << "\n";
	for (size_t i = 0; i < nthreads; ++i) {
		threads[i].join();
	}

	for (size_t i = 0; i < nthreads; ++i) {
		std::cerr << "thread #" << i << " tasks: " << taken[i].size() << "\n";
		for (auto t : taken[i]) {
			auto found = std::find(tasks.begin(), tasks.end(), t);

			if (found == tasks.end())
				std::cout << "Taks #" << t << " is taken twice" << std::endl;

			ASSERT_TRUE(found != tasks.end());
			tasks.erase(found);
		}
	}

	delete set;
	free(m);

}
