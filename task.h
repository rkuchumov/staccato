#ifndef TASK_H
#define TASK_H

#include <cstdlib>
#include <atomic>
#include <mutex>

class task
{
public:
	task();
	virtual ~task();

	virtual void execute() = 0;

	void set_subtask_count(size_t n);
	void spawn(task *t);
	void wait_for_all();

	task *parent();

private:
	friend class scheduler;

	std::atomic_size_t subtask_count;
	size_t get_subtask_count();
	void decrement_subtask_count();

#ifndef NDEBUG
	enum state_t {
		undef     = 0,
		ready     = 1,
		executing = 2,
		finished  = 3
	};
	state_t state;
#endif

	task *m_parent;
};

#endif /* end of include guard: TASK_H */

