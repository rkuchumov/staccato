#ifndef DISPATCHER_HPP_KVIXMHLF
#define DISPATCHER_HPP_KVIXMHLF

#include <cstdlib>
#include <thread>
#include <limits>

#include <pthread.h>

#include "utils.hpp"

#include "task_deque.hpp"
#include "lifo_allocator.hpp"
#include "task.hpp"
#include "counter.hpp"
#include "worker.hpp"

namespace staccato
{
namespace internal
{

template <typename T>
class dispatcher
{
public:
	dispatcher(
		size_t id,
		int core_id,
		lifo_allocator *alloc,
		size_t nworkers
	);

	~dispatcher();

	void register_worker(worker<T> *w);

	void stop();

	void loop();

private:
	struct worker_state {
		worker<T> *w;
		uint64_t load;
		size_t mailbox_size;
	};

	const size_t m_id;
	lifo_allocator *m_allocator;

	std::atomic_bool m_stopped;

	std::atomic_size_t m_nworkers;

	worker_state *m_workers;
};

template <typename T>
dispatcher<T>::dispatcher(
	size_t id,
	int core_id,
	lifo_allocator *alloc,
	size_t nworkers
)
: m_id(id)
, m_allocator(alloc)
, m_stopped(false)
, m_nworkers(0)
, m_workers(nullptr)
{
	m_workers = m_allocator->alloc_array<worker_state>(nworkers);

	if (core_id >= 0) {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core_id, &cpuset);

		pthread_t current_thread = pthread_self();    
		pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	}
}

template <typename T>
dispatcher<T>::~dispatcher()
{
	delete m_allocator;
}

template <typename T>
void dispatcher<T>::register_worker(worker<T> *w)
{
	m_workers[m_nworkers].w = w;
	m_workers[m_nworkers].load = 0;
	m_workers[m_nworkers].mailbox_size = 0;
	m_nworkers++;
}

template <typename T>
void dispatcher<T>::stop()
{
	m_stopped = true;
}

template <typename T>
void dispatcher<T>::loop()
{
	while (!load_relaxed(m_stopped)) {
		std::this_thread::yield();
	}
}

}
}

#endif /* end of include guard: DISPATCHER_HPP_KVIXMHLF */
