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
	task_deque(size_t log_size);
	~task_deque();

	void put(task *t);
	task *take();
	task *steal();

#if STACCATO_SAMPLE_DEQUES_SIZES
	size_t get_top();
#endif // STACCATO_SAMPLE_DEQUES_SIZES

private:
	typedef std::atomic<task *> atomic_task;
	typedef struct {
		size_t size_mask; // = (2^log_size) - 1
		atomic_task *buffer;
	} array_t;

	STACCATO_ALIGN std::atomic_size_t top;

	STACCATO_ALIGN std::atomic_size_t bottom;

	STACCATO_ALIGN std::atomic<array_t *> array;

	void resize();
};

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
