#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

// TODO: check if present
#include <pthread.h>

#include "constants.hpp"
#include "utils.hpp"
#include "worker.hpp"
#include "topology.hpp"

namespace staccato
{

template <typename T>
class task;

template <typename T>
class scheduler
{
public:

	scheduler (
		size_t nworkers,
		size_t taskgraph_degree,
		size_t taskgraph_height = 1,
		const topology &topo = topology() 
	);

	~scheduler();

	T *root();
	void spawn(T *t);
	void wait();

private:
	void create_workers(
		size_t taskgraph_degree,
		size_t taskgraph_height,
		const topology &topo);

	enum state_t { 
		terminated,
		initializing,
		initialized,
		terminating,
	};

	size_t m_nworkers;
	std::atomic<state_t> m_state;

	internal::worker<T> **m_workers;
};

template <typename T>
scheduler<T>::scheduler(
	size_t nworkers,
	size_t taskgraph_degree,
	size_t taskgraph_height,
	const topology &topo
)
: m_nworkers(nworkers)
, m_state(state_t::terminated)
{
	internal::Debug() << "Scheduler is working in debug mode";

	m_state = state_t::initializing;

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	create_workers(taskgraph_degree, taskgraph_height, topo);

	m_state = state_t::initialized;
}

template <typename T>
void scheduler<T>::create_workers(
	size_t taskgraph_degree,
	size_t taskgraph_height,
	const topology &topo
) {
	using namespace internal;

	auto degree = next_pow2(taskgraph_degree);
	m_workers = new worker<T> *[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i] = new worker<T>(degree, taskgraph_height);

	for (auto &n : topo.get()) {
		if (n.id >= m_nworkers)
			break;

		if (n.id == 0) {
			Debug() << "Init worker #" << n.id << " at CPU" << n.core << " without victim";
			m_workers[n.id]->async_init(true, n.core, nullptr);
		} else {
			Debug() << "Init worker #" << n.id << " at CPU" << n.core << " with victim #" << n.victim;
			m_workers[n.id]->async_init(false, n.core, m_workers[n.victim]);
		}
	}
}

template <typename T>
scheduler<T>::~scheduler()
{
	m_state = terminating;

	for (size_t i = 1; i < m_nworkers; ++i)
		m_workers[i]->join();

	for (size_t i = 0; i < m_nworkers; ++i)
		delete m_workers[i];

	delete []m_workers;

	m_state = terminated;
}

template <typename T>
T *scheduler<T>::root()
{
	return m_workers[0]->root_allocate();
}

template <typename T>
void scheduler<T>::spawn(T *)
{
	m_workers[0]->root_commit();
}

template <typename T>
void scheduler<T>::wait()
{
	m_workers[0]->root_wait();
}

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

