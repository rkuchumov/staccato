#include "utils.h"
#include "worker.h"
#include "scheduler.h"
#include "constants.h"

#include <unistd.h>


namespace staccato
{
namespace internal
{

worker::worker(size_t deque_log_size)
: pool(deque_log_size)
{

}

worker::~worker()
{

}

void worker::fork()
{
	std::cerr << "forking thread\n";
	handle = new std::thread([=] {task_loop();});
}

void worker::join()
{

}

void worker::task_loop(task *waiting, task *t)
{
	// ASSERT(parent != nullptr || (parent == nullptr && my_pool != pool),
	// 		"Root task must be executed only on master");

	while (true) {
		while (true) { // Local tasks loop
			if (t) {

// #if STACCATO_DEBUG
// 				ASSERT(t->get_state() == task::taken || t->get_state() == task::stolen,
// 					"Incorrect task state: " << t->get_state_str());
// 				t->set_state(task::executing);
// #endif // STACCATO_DEBUG

				t->set_executer(this);
				t->execute();

// #if STACCATO_DEBUG
// 				ASSERT(t->get_state() == task::executing,
// 					"Incorrect task state: " << t->get_state_str());
// 				t->set_state(task::finished);
//
// 				ASSERT(t->subtask_count == 0,
//
// 						"Task still has subtaks after it has been executed");
// #endif // STACCATO_DEBUG

				// if (t->parent != nullptr)
					// dec_relaxed(t->parent->subtask_count);

				if (t->parent != nullptr)
					dec_relaxed(t->parent->subtask_count);
			}

			if (waiting != nullptr && load_relaxed(waiting->subtask_count) == 0)
				return;
			// if (waiting != nullptr && load_relaxed(waiting->subtask_count) == 0)
			// 	return;

			t = pool.take();

			if (!t){

				std::cerr << "atat" << "\n";
				break;
			}
		} 

		// std::cerr << std::this_thread::get_id() << "\n";
		// std::cerr << scheduler::is_active() << "\n";

		// if (waiting == nullptr && !scheduler::is_active())
			// return;

		auto victim = scheduler::get_victim(this);

		t = victim->pool.steal();

		if (t)
			std::cout << t << "stolen" << std::endl;

		// sleep(3);
	} 

	ASSERT(false, "Must never get there");
}

void worker::enqueue(task *t)
{
	// std::cout << "put " << t << std::endl;
	pool.put(t);
}

}
}
