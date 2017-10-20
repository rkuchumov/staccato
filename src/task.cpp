#include "utils.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "worker.hpp"

namespace staccato
{

task::task()
: executer(nullptr)
, subtask_count(0)
, parent_subtask_count(nullptr)
, next(nullptr)
{ }

task::~task()
{ }

void task::spawn(task *t)
{
	ASSERT(t->subtask_count == 0,
		"Spawned task is not allowed to have subtasks");

	t->parent_subtask_count = &subtask_count;

	inc_relaxed(subtask_count);

	ASSERT(executer != nullptr, "Executed by nullptr");

	executer->pool.put(t);
}

void task::wait()
{
	ASSERT(executer != nullptr, "Executed by nullptr");

	executer->task_loop(this);

	ASSERT(subtask_count == 0,
		"Task still has subtaks after task_loop()");
}

void task::process(uint8_t *t)
{
	ASSERT(t, "Processing nullptr");

	t->executer = this;
	t->execute();

	ASSERT(
		t->subtask_count == 0,
		"Task still has subtaks after it has been executed"
	);

	if (t->parent_subtask_count != nullptr)
		dec_relaxed(*t->parent_subtask_count);
}

bool task::has_finished(uint8_t *t)
{
	ASSERT(t, "Processing nullptr");

	return (load_relaxed(waiting->subtask_count) == 0);
}

void task::then(task *t)
{
	// TODO: append to next
	next = t;
}

void *task::operator new(size_t sz)
{
	return std::malloc(sz);
}

void task::operator delete(void *ptr) noexcept
{
	std::free(ptr);
}

} // namespace stacccato
