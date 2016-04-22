#include "utils.h"
#include "task.h"
#include "scheduler.h"

namespace staccato
{

task::~task()
{

}

void task::spawn(task *t)
{
	t->parent = this;
	inc_relaxed(subtask_count);
	scheduler::spawn(t);
}

void task::wait_for_all()
{
	scheduler::task_loop(/*parent=*/this);
	ASSERT(subtask_count == 0, "Task still has subtaks after it task_loop()");
}

task::task(): parent(nullptr), subtask_count(0)
{
#if STACCATO_DEBUG
	state = ready;
#endif // STACCATO_DEBUG
}

} // namespace stacccato
