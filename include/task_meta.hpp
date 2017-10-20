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

class task_meta {
public:
	task_meta();
	virtual ~task_meta();

	static void process(uint8_t *task, internal::worker *executer);

	static bool has_finished(uint8_t *task);

	void spawn(uint8_t *task);

	void wait();

	static size_t task_size;

	static std::function<void(uint8_t *)> handler;

	static uint8_t *stack_allocate();

// private:
	// internal::task_deque *parent_pool;

	internal::worker *executer;

	std::atomic_size_t subtask_count;
	std::atomic_size_t *parent_subtask_count;
};


} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
