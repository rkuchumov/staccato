#ifndef STACCATO_TASK_H
#define STACCATO_TASK_H

#include <cstdlib>
#include <atomic>
#include <ostream>

#include "task_base.hpp"
#include "constants.hpp"

namespace staccato
{

class scheduler;

namespace internal {
class worker;
}

class task : public task_base<task>
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

	static void process(uint8_t *t);
	static bool has_finished(uint8_t *t);

private:
	friend class internal::worker;
	friend class scheduler;

	internal::worker *executer;

	std::atomic_size_t subtask_count;
	std::atomic_size_t *parent_subtask_count;

	task *next;
};

}

#endif /* end of include guard: STACCATO_TASK_H */
