#include "utils.h"
#include "task.h"
#include "scheduler.h"

namespace staccato
{

task::task()
: parent(nullptr)
, executer(nullptr)
, subtask_count(0)
#if STACCATO_DEBUG
, state(initializing)
#endif // STACCATO_DEBUG
{
	// std::cout << "!!!" << parent << std::endl;
}

task::~task()
{ }

void task::spawn(task *t)
{
	// ASSERT(t->subtask_count == 0,
	// 	"Spawned task is not allowed to have subtasks");
	// ASSERT(parent == nullptr || (parent != nullptr && state == executing),
	// 	"Incorrect current task state: " << t->get_state_str());
	// ASSERT(t->state == initializing,
	// 	"Incorrect newtask state: " << t->get_state_str());

#if STACCATO_DEBUG
	t->state = spawning;
#endif // STACCATO_DEBUG
	// std::cerr << "t: : " << this << std::endl;

	t->parent = this;

	inc_relaxed(subtask_count);

	executer->enqueue(t);
}

void task::wait_for_all()
{
	// ASSERT(parent == nullptr || (parent != nullptr && state == executing),
		// "Incorrect current task state: " << get_state_str());

	ASSERT(executer != nullptr, "Executed by nullptr");

	// TODO: wrap this function, it should be private
	executer->task_loop(this);

	// scheduler::task_loop(#<{(|parent=|)}>#this);

	ASSERT(subtask_count == 0,
		"Task still has subtaks after task_loop()");
}

void task::set_executer(internal::worker *worker)
{
	executer = worker;
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

const char *task::get_state_str()
{
	switch (state) {
		case undefined    : return "undefined";
		case initializing : return "initializing";
		case spawning     : return "spawning";
		case ready        : return "ready";
		case taken        : return "taken";
		case stolen       : return "stolen";
		case executing    : return "executing";
		case finished     : return "finished";
		default           : break;
	}

	ASSERT(false, "String representation for " << state << " is not defined");
	return nullptr;
}

#endif // STACCATO_DEBUG

} // namespace stacccato
