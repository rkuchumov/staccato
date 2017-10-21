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
, handle(nullptr)
, m_ready(false)
, deque(deque_log_size, elem_size)
, stack(deque_log_size, elem_size)
{ }

worker::~worker()
{
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
	auto c = stack.allocate();

	while (true) {
		while (true) { // Local tasks loop
			if (t)
				task::process(this, t);

			if (waiting && task::has_finished(waiting))
			{
				stack.deallocate();
				return;
			}

			t = deque.take(c);

			if (!t)
				break;
		}

		auto state = load_relaxed(scheduler::state);
		if (state == scheduler::terminating)
			return;

		auto victim = scheduler::get_victim(this);

		t = victim->deque.steal(c);
	} 

	ASSERT(false, "Must never get there");
}

uint8_t *worker::put_allocate()
{
	return deque.put_allocate();
}

void worker::put_commit()
{
	deque.put_commit();
}

}
}
