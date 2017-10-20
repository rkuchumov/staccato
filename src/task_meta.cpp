#include <iostream>
#include <cstring>

#include <alloca.h>

#include "task_deque.hpp"
#include "task_meta.hpp"
#include "worker.hpp"

namespace staccato
{

std::function<void(uint8_t *)> task_meta::handler;
size_t task_meta::task_size;

task_meta::task_meta()
	: executer(nullptr)
	, subtask_count(0)
	, parent_subtask_count(nullptr)
{

}

task_meta::~task_meta()
{

}

void task_meta::process(uint8_t *raw, internal::worker *executer)
{
	ASSERT(raw, "Processing nullptr");

	auto task = reinterpret_cast<task_meta *>(raw);

	task->executer = executer;
	// task->parent_pool = pool;

	ASSERT(handler, "Task handler is not set");
	handler(raw);

	ASSERT(
		task->subtask_count == 0,
		"Task still has subtaks after it has been executed"
	);

	if (task->parent_subtask_count != nullptr)
		dec_relaxed(*task->parent_subtask_count);

	delete raw;
}

bool task_meta::has_finished(uint8_t *raw)
{
	ASSERT(raw, "Processing nullptr");
	auto task = reinterpret_cast<task_meta *>(raw);

	return load_relaxed(task->subtask_count) == 0;
}

void task_meta::spawn(uint8_t *raw)
{
	auto task = reinterpret_cast<task_meta *>(raw);

	ASSERT(task->subtask_count == 0,
		"Spawned task is not allowed to have subtasks");

	inc_relaxed(subtask_count);

	ASSERT(executer != nullptr, "Executed by nullptr");
	auto r = executer->pool.put_allocate();

	task->parent_subtask_count = &subtask_count;

	std::memcpy(r, raw, task_size);

	executer->pool.put_commit();
}

void task_meta::wait()
{
	ASSERT(executer != nullptr, "Executed by nullptr");

	executer->task_loop(reinterpret_cast<uint8_t *>(this));

	ASSERT(subtask_count == 0,
		"Task still has subtaks after task_loop()");
}

uint8_t *task_meta::stack_allocate()
{
	return static_cast<uint8_t *> (std::malloc(task_size));
	// return static_cast<uint8_t *> (alloca(task_size));
}

} /* staccato */ 
