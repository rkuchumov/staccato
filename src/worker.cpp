#include "utils.hpp"
#include "worker.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "constants.hpp"

namespace staccato
{
namespace internal
{

worker::worker(size_t deque_log_size)
: pool(deque_log_size)
, handle(nullptr)
, m_ready(false)
{

}

worker::~worker()
{
	if (handle) {
		delete handle;
	}
}

void worker::fork()
{
	handle = new std::thread([=] {
			m_ready = true;
			scheduler::wait_until_initilized();
			Debug() << "Starting worker::tasks_loop()";
			task_loop();
		}
	);
}

bool worker::ready() const
{
	return m_ready;
}

void worker::join()
{
	ASSERT(handle, "Thread is not created");
	handle->join();
}

void worker::task_loop(task *waiting, task *t)
{
	while (true) {
		while (true) { // Local tasks loop
			if (t) {
				for (auto s = t; s; s = s->next) {
					s->executer = this;
					s->execute();
				}

				ASSERT(
					t->subtask_count == 0,
					"Task still has subtaks after it has been executed"
				);

				if (t->parent != nullptr)
					dec_relaxed(t->parent->subtask_count);
			}

			if (waiting != nullptr && load_relaxed(waiting->subtask_count) == 0)
				return;

			t = pool.take();

			if (!t) {
				break;
			}
		} 

		auto state = load_relaxed(scheduler::state);
		if (state == scheduler::terminating) {
			Debug() << "Worker exiting (empty queue, scheduler is terminated)";
			return;
		}

		auto victim = scheduler::get_victim(this);

		t = victim->pool.steal();
	} 

	ASSERT(false, "Must never get there");
}

}
}
