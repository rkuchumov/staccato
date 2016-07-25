#ifndef STACCATO_DEQUE_H
#define STACCATO_DEQUE_H

#include "constants.h"
#include "task.h"

namespace staccato
{
namespace internal
{

class task_deque
{
public:
	task_deque();

	void put(task *t);
	task *take();
	task *steal(task_deque *thief);

	static size_t deque_log_size;
	static size_t tasks_per_steal;

#if STACCATO_SAMPLE_DEQUES_SIZES
	size_t get_top();
#endif // STACCATO_SAMPLE_DEQUES_SIZES

private:
	typedef std::atomic<task *> atomic_task;
	typedef struct {
		size_t size_mask; // = (2^log_size) - 1
		atomic_task *buffer;
	} array_t;

	struct stamped_size_t
	{
		size_t value;
		size_t stamp;
	};

	STACCATO_ALIGN std::atomic<stamped_size_t> top;

	STACCATO_ALIGN std::atomic_size_t bottom;

	STACCATO_ALIGN std::atomic<array_t *> array;

	size_t load_from(array_t *buffer, size_t top, size_t s);

	void resize();
};

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
