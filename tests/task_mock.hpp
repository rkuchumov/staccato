#ifndef TASK_MOCK_HPP_FKQ408NU
#define TASK_MOCK_HPP_FKQ408NU

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "task_meta.hpp"

using namespace ::testing;

class task_mock: public staccato::task_meta
{
public:
	task_mock(size_t id_) : id(id_)
	{};

	virtual ~task_mock() {};

	size_t id;

	// MOCK_METHOD0(execute, void());
};

#endif /* end of include guard: TASK_MOCK_HPP_FKQ408NU */
