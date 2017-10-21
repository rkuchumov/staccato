#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

#include "task_deque.hpp"
#include "task_stack.hpp"
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

	uint8_t *put_allocate();
	void put_commit();

private:
	std::thread *handle;
	std::atomic_bool m_ready;

	task_deque deque;

	task_stack stack;
};

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
