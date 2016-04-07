#include "utils.h"
#include "task.h"
#include "scheduler.h"

void task::spawn(task *t)
{
	t->m_parent = this;
	scheduler::spawn(t);
}

void task::wait_for_all()
{
	scheduler::task_loop(/*parent=*/this);
	ASSERT(subtask_count == 0, "Task still has subtaks after it task_loop()");
}

task *task::parent()
{
	return m_parent;
}

task::task() 
{
	subtask_count = 0;
	m_parent = NULL;

#ifndef NDEBUG
	state = ready;
#endif
}

task::~task()
{

}

void task::set_subtask_count(size_t n)
{
	subtask_count.store(n);
}

size_t task::get_subtask_count()
{
	return subtask_count.load();
}

void task::decrement_subtask_count()
{
	subtask_count.fetch_sub(1);
}
