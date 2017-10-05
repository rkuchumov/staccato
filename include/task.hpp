#ifndef STACCATO_TASK_H
#define STACCATO_TASK_H

#include <cstdlib>
#include <atomic>
#include <ostream>

#include "constants.hpp"

namespace staccato
{

class scheduler;

namespace internal {
class worker;
}

class task
{
public:
	task();
	virtual ~task();

	void spawn(task *t);
	void wait();

	void then(task *t);

	virtual void execute() = 0;

	void *operator new(size_t sz);

	void operator delete(void *ptr) noexcept;

private:
	friend class internal::worker;
	friend class scheduler;

	task *parent;

	internal::worker *executer;

	std::atomic_size_t subtask_count;

	task *next;

};

}

#endif /* end of include guard: STACCATO_TASK_H */
