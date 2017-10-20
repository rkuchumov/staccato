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
class worker;
}

class task {
public:
	task();
	virtual ~task();

	virtual void execute() = 0;

	uint8_t *child();

	void spawn(task *t);

	void wait();

private:
	friend class internal::worker;

	static void process(internal::worker *executer, uint8_t *raw);
	static bool has_finished(uint8_t *raw);

	internal::worker *executer;

	std::atomic_size_t subtask_count;
	std::atomic_size_t *parent_subtask_count;
};


} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
