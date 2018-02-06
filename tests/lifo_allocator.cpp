#include <list>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lifo_allocator.hpp"

using namespace staccato;
using namespace staccato::internal;

TEST(ctor, creating_and_deleteing) {
	auto a = new lifo_allocator(100);
	delete a;
}

TEST(alloc_dealloc, no_reize) {
	size_t n = 5000;
	auto a = new lifo_allocator(2048);

	std::vector<size_t *> ptrs;

	for (size_t i = 1; i <= n; ++i) {
		auto p = new(a->alloc<size_t>()) size_t;
		*p = i;
		ptrs.push_back(p);
	}

	for (size_t i = n; i > 1; --i) {
		auto p = ptrs.back();
		EXPECT_EQ(*p, i);

		ptrs.pop_back();
	}

	delete a;
}

