#include "utils.h"
#include "task.h"
#include "scheduler.h"
#include "worker.h"

namespace staccato
{

task::task()
#if STACCATO_DEBUG
: state(initializing)
, parent(nullptr)
#else
: parent(nullptr)
#endif // STACCATO_DEBUG
, executer(nullptr)
, subtask_count(0)
{
}

task::~task()
{ }

void task::spawn(task *t)
{
	ASSERT(t->subtask_count == 0,
		"Spawned task is not allowed to have subtasks");
	ASSERT(t->state == initializing,
		"Incorrect newtask state: " << t->get_state());

#if STACCATO_DEBUG
	t->state = spawning;
#endif // STACCATO_DEBUG

	t->parent = this;

	inc_relaxed(subtask_count);

	executer->pool.put(t);
}

void task::wait_for_all()
{
	// ASSERT(parent == nullptr || (parent != nullptr && state == executing),
		// "Incorrect current task state: " << get_state());

	ASSERT(executer != nullptr, "Executed by nullptr");

	executer->task_loop(this);

	ASSERT(subtask_count == 0,
		"Task still has subtaks after task_loop()");
}

#if STACCATO_DEBUG
void task::set_state(unsigned s)
{
	state = s;
}

unsigned task::get_state()
{
	return state;
}

std::ostream& operator<<(std::ostream & os, task::task_state &state)
{
	switch (state) {
		case task::task_state::undefined    : os << "undefined"    ; break ;
		case task::task_state::initializing : os << "initializing" ; break ;
		case task::task_state::spawning     : os << "spawning"     ; break ;
		case task::task_state::ready        : os << "ready"        ; break ;
		case task::task_state::taken        : os << "taken"        ; break ;
		case task::task_state::stolen       : os << "stolen"       ; break ;
		case task::task_state::executing    : os << "executing"    ; break ;
		case task::task_state::finished     : os << "finished"     ; break ;
		default                             : break;
	}

	ASSERT(false, "String representation for " << state << " is not defined");
	return os;
}


#endif // STACCATO_DEBUG

} // namespace stacccato
