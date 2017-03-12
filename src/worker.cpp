#include "utils.h"
#include "worker.h"
#include "task.h"
#include "scheduler.h"
#include "constants.h"

namespace staccato
{
namespace internal
{

worker::worker(size_t deque_log_size)
: pool(deque_log_size)
, handle(nullptr)
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
	handle = new std::thread([=] {task_loop();});
}

void worker::join()
{
	ASSERT(handle, "Thread is not created");
	handle->join();
}

void worker::task_loop(task *waiting, task *t)
{
	scheduler::wait_workers_fork();

	while (true) {
		while (true) { // Local tasks loop
			if (t) {

#if STACCATO_DEBUG
				ASSERT(t->get_state() == task::taken || t->get_state() == task::stolen,
					"Incorrect task state: " << t->get_state_str());
				t->set_state(task::executing);
#endif // STACCATO_DEBUG

				t->executer = this;
				t->execute();

#if STACCATO_DEBUG
				ASSERT(t->get_state() == task::executing,
					"Incorrect task state: " << t->get_state_str());
				t->set_state(task::finished);

				ASSERT(t->subtask_count == 0,
						"Task still has subtaks after it has been executed");
#endif // STACCATO_DEBUG

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

		if (waiting == nullptr && !load_relaxed(scheduler::is_active))
			return;

		auto victim = scheduler::get_victim(this);

		t = victim->pool.steal();
	} 

	ASSERT(false, "Must never get there");
}

}
}
