#include <list>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "task_mock.hpp"
#include "task_mailbox.hpp"

using namespace staccato;
using namespace staccato::internal;

static const size_t test_timeout = 2;
static const size_t nthreads = 4;

TEST(ctor, creating_and_deleteing) {
	auto d = new task_mailbox<task_mock, 8>();
	delete d;
}

TEST(take, single) {
	size_t ntasks = 8;
	auto mailbox = new task_mailbox<task_mock, 8>();

	for (size_t i = 1; i <= ntasks; ++i) {
		mailbox->put(new task_mock(i));
	}

	for (size_t i = 1; i <= ntasks; ++i) {
		task_mock *t = mailbox->take();
		ASSERT_EQ(t->id, i);
		ASSERT_EQ(mailbox->size(), ntasks - i);
		delete t;
	}

	delete mailbox;
}

TEST(steal, concurrent_take_and_put) {
	size_t ntasks = 1 << 13;

	auto mailbox = new task_mailbox<task_mock, 1 << 13>();

	std::vector<size_t> tasks;
	std::vector<size_t > taken;

	std::atomic_bool stop(false);
	std::atomic_int nready(0);

	auto take_op = [&]() {
		nready++;

		while (nready != 2)
			std::this_thread::yield();

		while (!stop) {
			auto t = mailbox->take();
			if (t) {
				taken.push_back(t->id);
				delete t;
			}
		}
	};

	auto put_op = [&]() {
		nready++;

		while (nready != 2)
			std::this_thread::yield();

		for (size_t i = 1; i <= ntasks; ++i) {
			mailbox->put(new task_mock(i));
			tasks.push_back(i);
		}
	};

	std::thread take_thread = std::thread(take_op);
	std::thread put_thread = std::thread(put_op);

	std::this_thread::sleep_for(std::chrono::seconds(test_timeout));
	stop = true;

	std::cerr << "total tasks: " << ntasks << "\n";
	take_thread.join();
	put_thread.join();

	for (auto t : taken) {
		auto found = std::find(tasks.begin(), tasks.end(), t);
		ASSERT_TRUE(found != tasks.end());
		tasks.erase(found);
	}

	EXPECT_EQ(mailbox->size(), 0);

	delete mailbox;
}

