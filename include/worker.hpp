#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

#include "task_deque.hpp"
#include "constants.hpp"

namespace staccato
{

class task;

namespace internal
{

class worker
{
public:
	worker(size_t deque_log_size);
	~worker();

	void fork();
	void join();

	void task_loop(task *parent = nullptr, task *t = nullptr);

	task_deque pool;

	bool ready() const;

private:
	std::thread *handle;
	std::atomic_bool m_ready;

};

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
