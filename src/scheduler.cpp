#include "utils.hpp"
#include "scheduler.hpp"
#include "worker.hpp"
#include "task.hpp"
#include "root_task.hpp"
#include "lambda_task.hpp"


namespace staccato
{
using namespace internal;

worker **scheduler::workers;
size_t scheduler::workers_count(0);
std::atomic<scheduler::state_t> scheduler::state(terminated);
task *scheduler::m_root(nullptr);

scheduler::scheduler()
{ }

scheduler::~scheduler()
{ }

void scheduler::initialize(
	size_t task_size,
	size_t nthreads,
	size_t deque_log_size)
{
	ASSERT(
		state == state_t::terminated,
		"Schdeler is already initialized"
	);

	ASSERT(task_size >= sizeof(root_task),
		"The of a task is too small");

	state = state_t::initializing;

	if (nthreads == 0)
		nthreads = std::thread::hardware_concurrency();
	workers_count = nthreads;

	workers = new worker *[workers_count];
	for (size_t i = 0; i < workers_count; ++i)
		workers[i] = new worker(deque_log_size, task_size);

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->fork();

	wait_workers_fork();

	m_root = new root_task();
	m_root->executer = workers[0];

	state = state_t::initialized;
}

void scheduler::wait_workers_fork()
{
	for (size_t i = 1; i < workers_count; ++i) {
		if (workers[i]->ready())
			continue;

		do {
			std::this_thread::yield();
		} while (!workers[i]->ready());
	}
}

void scheduler::wait_until_initilized()
{
	ASSERT(
		state == state_t::initializing || state_t::initialized,
		"Incorrect scheduler state"
	);

	while (load_consume(state) == state_t::initializing)
		std::this_thread::yield();

	ASSERT(
		state != state_t::initializing,
		"Incorrect scheduler state"
	);
}

void scheduler::terminate()
{
	ASSERT(
		state == state_t::initialized,
		"Scheduler is not initialized"
	);
	state = terminating;

	for (size_t i = 1; i < workers_count; ++i)
		workers[i]->join();

	for (size_t i = 0; i < workers_count; ++i)
		delete workers[i];

	delete []workers;

	state = terminated;
}

uint8_t *scheduler::root()
{
	ASSERT(m_root, "Root task is not allocated");
	return m_root->child();
}

void scheduler::spawn(task *t)
{
	ASSERT(m_root, "Root task is not allocated");
	m_root->spawn(t);
}

void scheduler::spawn(std::function<void ()> fn)
{
	ASSERT(m_root, "Root task is not allocated");
	m_root->spawn(new(m_root->child()) lambda_task(fn));
}

void scheduler::wait()
{
	ASSERT(m_root, "Root task is not initialized");
	m_root->wait();
}

worker *scheduler::get_victim(worker *thief)
{
	ASSERT(
		workers_count,
		"Stealing when scheduler has a single worker"
	);

	size_t n = workers_count;
	size_t i = xorshift_rand() % n;
	auto victim = workers[i];
	if (victim == thief) {
		i++;
		i %= n;
		victim = workers[i];
	}

	return victim;
}

} // namespace stacccato
