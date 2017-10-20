#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

#include "task_deque.hpp"
#include "constants.hpp"

namespace staccato
{

namespace internal
{

class worker
{
public:
	worker(size_t deque_log_size, size_t elem_size);
	~worker();

	void fork();
	void join();

	void task_loop(uint8_t *parent = nullptr, uint8_t *t = nullptr);

	bool ready() const;

	const size_t elem_size;

	uint8_t *task_stack;
	size_t task_stack_end;

	task_deque pool;

	std::thread *handle;
	std::atomic_bool m_ready;
};

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
