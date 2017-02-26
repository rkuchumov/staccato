#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

#include "task_deque.h"
#include "constants.h"

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

private:
	std::thread *handle;
};

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
