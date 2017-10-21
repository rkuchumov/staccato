#include <iostream>
#include <cstring>

#include "task_deque.hpp"
#include "task.hpp"
#include "worker.hpp"

#include "utils.hpp"

namespace staccato
{

task::task()
: executer(nullptr)
, subtask_count(0)
, parent_subtask_count(nullptr)
{ }

task::~task()
{ }

uint8_t *task::child()
{
	ASSERT(executer != nullptr, "Executed by nullptr");
	return executer->put_allocate();
}

void task::spawn(task *t)
{
	ASSERT(t->subtask_count == 0,
		"Spawned task is not allowed to have subtasks");
	ASSERT(child() == reinterpret_cast<uint8_t *> (t),
		"task::spawn() must be called after allocation");

	inc_relaxed(subtask_count);

	t->parent_subtask_count = &subtask_count;

	executer->put_commit();
}

void task::process(internal::worker *executer, uint8_t *raw)
{
	ASSERT(raw, "Processing nullptr");
	auto t = reinterpret_cast<task *>(raw);

	t->executer = executer;

	t->execute();

	ASSERT(t->subtask_count == 0,
		"Task still has subtaks after it has been executed");

	if (t->parent_subtask_count != nullptr)
		dec_relaxed(*t->parent_subtask_count);
}

void task::wait()
{
	ASSERT(executer, "Executed by nullptr");

	executer->task_loop(reinterpret_cast<uint8_t *>(this));

	ASSERT(subtask_count == 0,
		"Task still have subtaks after task_loop()");
}

bool task::has_finished(uint8_t *raw)
{
	ASSERT(raw, "Processing nullptr");
	auto t = reinterpret_cast<task *>(raw);

	return load_relaxed(t->subtask_count) == 0;
}

} /* staccato */ 
