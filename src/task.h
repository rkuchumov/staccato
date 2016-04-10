#ifndef TASK_H
#define TASK_H

#include <cstdlib>
#include <atomic>

class task
{
public:
	task();
	virtual ~task() {};

	void spawn(task *t);
	void wait_for_all();

	virtual void execute() = 0;

private:
	friend class scheduler;

	task *parent;

	std::atomic_size_t subtask_count;

#ifndef NDEBUG
	enum state_t {
		undef     = 0,
		ready     = 1,
		executing = 2,
		finished  = 3
	};
	state_t state;
#endif
};

#endif /* end of include guard: TASK_H */

