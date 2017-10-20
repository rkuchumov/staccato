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
	auto d = new task_deque(4, 8);
	delete d;
}

TEST(take, without_resize) {
	size_t size = 5;
	size_t ntasks = 1 << (size - 1);

	auto deque = new task_deque(size, sizeof(task_mock));

	for (size_t i = 1; i <= ntasks; ++i) {
		new(deque->put_allocate()) task_mock(i);
		deque->put_commit();
	}

	task_mock t(10000);
	for (size_t i = ntasks; i >= 1; --i) {
		deque->take(reinterpret_cast<uint8_t *> (&t));
		EXPECT_EQ(t.id, i);
	}

	delete deque;
}

TEST(take, with_resize) {
	size_t size = 5;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size, sizeof(task_mock));

	for (size_t i = 1; i <= ntasks; ++i) {
		new(deque->put_allocate()) task_mock(i);
		deque->put_commit();
	}

	task_mock t(10000);
	for (size_t i = ntasks; i >= 1; --i) {
		deque->take(reinterpret_cast<uint8_t *> (&t));
		EXPECT_EQ(t.id, i);
	}

	delete deque;
}

TEST(steal, sequential) {
	size_t size = 5;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size, sizeof(task_mock));

	for (size_t i = 1; i <= ntasks; ++i) {
		new(deque->put_allocate()) task_mock(i);
		deque->put_commit();
	}

	task_mock t(10000);
	for (size_t i = 1; i <= ntasks; ++i) {
		deque->steal(reinterpret_cast<uint8_t *> (&t));
		EXPECT_EQ(t.id, i);
	}

	delete deque;
}

TEST(steal, concurrent_steal) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	std::vector<size_t> tasks;

	auto deque = new task_deque(size, sizeof(task_mock));

	for (size_t i = 1; i <= ntasks; ++i) {
		new(deque->put_allocate()) task_mock(i);
		deque->put_commit();
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

		task_mock mem(10000);
		while (!stop) {
			auto t = deque->steal(reinterpret_cast<uint8_t *> (&mem));
			if (t)
				stolen[id].push_back(mem.id);
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

	delete deque;
}

TEST(steal_take, concurrent_steal_and_take) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	auto deque = new task_deque(size, sizeof(task_mock));

	std::vector<size_t> tasks;

	for (size_t i = 1; i <= ntasks; ++i) {
		new(deque->put_allocate()) task_mock(i);
		deque->put_commit();
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

		task_mock mem(10000);
		auto ptr = reinterpret_cast<uint8_t *> (&mem);
		while (!stop) {
			auto t = steal ? deque->steal(ptr) : deque->take(ptr);
			if (t)
				stolen[id].push_back(mem.id);
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

	EXPECT_TRUE(tasks.empty());

	delete deque;
}


TEST(put_steal_take, concurrent_steal_take_and_put) {
	size_t size = 12;
	size_t ntasks = 1 << (size + 2);

	std::vector<size_t> tasks;
	auto deque = new task_deque(size, sizeof(task_mock));

	std::atomic_size_t nready(0);
	std::atomic_bool stop(false);
	std::atomic_size_t tasks_left(ntasks);

	std::vector<size_t> stolen[nthreads];
	std::vector<size_t> taken;
	std::thread threads[nthreads];

	auto steal = [&](size_t id, bool steal) {
		nready++;

		while (nready != nthreads)
			std::this_thread::yield();

		while (!stop) {

			task_mock mem(10000);
			auto ptr = reinterpret_cast<uint8_t *> (&mem);

			if (steal) {
				auto t = deque->steal(ptr);
				if (t)
					stolen[id].push_back(mem.id);
			} else {
				if (tasks_left > 0) {
					new(deque->put_allocate()) task_mock(tasks_left);
					deque->put_commit();
					tasks.push_back(tasks_left);
					tasks_left--;
				}

				auto t = deque->take(ptr);
				if (t)
					stolen[id].push_back(mem.id);
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

	EXPECT_TRUE(tasks.empty());

	delete deque;
}
