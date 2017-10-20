#include "utils.hpp"
#include "worker.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "constants.hpp"

namespace staccato
{
namespace internal
{

worker::worker(size_t deque_log_size, size_t elem_size)
: elem_size(elem_size)
, task_stack_end(0)
, pool(deque_log_size, elem_size)
, handle(nullptr)
, m_ready(false)
{
	task_stack = new uint8_t[(1 << deque_log_size) * elem_size];
}

worker::~worker()
{
	delete task_stack;

	if (handle)
		delete handle;
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

void worker::task_loop(uint8_t *waiting, uint8_t *t)
{
	auto c = task_stack + task_stack_end * elem_size;
	task_stack_end++;

	while (true) {
		while (true) { // Local tasks loop
			if (t)
				task::process(this, t);

			if (waiting && task::has_finished(waiting))
			{
				task_stack_end--;
				return;
			}

			t = pool.take(c);

			if (!t)
				break;
		}

		auto state = load_relaxed(scheduler::state);
		if (state == scheduler::terminating)
			return;

		auto victim = scheduler::get_victim(this);

		t = victim->pool.steal(c);
	} 

	ASSERT(false, "Must never get there");
}

}
}
