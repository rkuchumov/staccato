#ifndef STACCATO_TASK_H
#define STACCATO_TASK_H

#include <cstdlib>
#include <atomic>

#include "constants.h"

namespace staccato
{

class task
{
public:
	task();
	virtual ~task();

	void spawn(task *t);
	void wait_for_all();

	virtual void execute() = 0;

private:
	friend class scheduler;

	task *parent;

	std::atomic_size_t subtask_count;

#if STACCATO_DEBUG
	// TODO: moar states
	enum state_t {
		undef     = 0,
		ready     = 1,
		executing = 2,
		finished  = 3
	};
	state_t state;
#endif // STACCATO_DEBUG

};

}

#endif /* end of include guard: STACCATO_TASK_H */
