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
: pool(deque_log_size, elem_size)
, handle(nullptr)
, m_ready(false)
{ }

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

void worker::task_loop(uint8_t *waiting, uint8_t *t)
{
	auto current = task::stack_allocate();
	// std::cerr << (void *) current << "\n";

	while (true) {
		while (true) { // Local tasks loop
			if (t)
				task::process(t, this);

			if (waiting && task::has_finished(waiting))
				return;

			t = pool.take(current);

			if (!t)
				break;
		}

		auto state = load_relaxed(scheduler::state);
		if (state == scheduler::terminating)
			return;

		auto victim = scheduler::get_victim(this);

		t = victim->pool.steal(current);
	} 

	ASSERT(false, "Must never get there");
}

}
}
