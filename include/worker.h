#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

// #include "task.h"
#include "deque.h"
#include "constants.h"

namespace staccato
{
namespace internal
{

class worker
{
public:
	worker(size_t deque_log_size);
	~worker();

	void fork();
	void join();

	void enqueue(task *t);
	void task_loop(task *parent = nullptr, task *t = nullptr);

private:
	friend class task;

	internal::task_deque pool;

	task *steal_task();

	std::thread *handle;

};

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
