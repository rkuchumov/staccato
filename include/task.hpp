#ifndef TASK_META_HPP_34TJDRLS
#define TASK_META_HPP_34TJDRLS

#include <atomic>
#include <cstdint>
#include <functional>
#include <cstdlib>

#include "constants.hpp"

namespace staccato
{

namespace internal {
class task_deque;
class worker;
}

class task {
public:
	task();
	virtual ~task();

	static void process(uint8_t *task, internal::worker *executer);

	static bool has_finished(uint8_t *task);

	uint8_t *child();

	void spawn(task *t);

	void wait();

	virtual void execute() = 0;

	static size_t task_size;

	static uint8_t *stack_allocate();

// private:
	// internal::task_deque *parent_pool;

	internal::worker *executer;

	std::atomic_size_t subtask_count;
	std::atomic_size_t *parent_subtask_count;
};


} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
