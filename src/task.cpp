#include "utils.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "worker.hpp"

namespace staccato
{

task::task()
: parent(nullptr)
, executer(nullptr)
, subtask_count(0)
, next(nullptr)
{ }

task::~task()
{ }

void task::spawn(task *t)
{
	ASSERT(t->subtask_count == 0,
		"Spawned task is not allowed to have subtasks");

	t->parent = this;

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
