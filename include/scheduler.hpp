#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>
#include <atomic>
#include <functional>

#include "constants.hpp"
#include "worker.hpp"

namespace staccato
{

template <typename T>
class task;

template <typename T>
class scheduler
{
public:
	scheduler(
		size_t nworkers,
		size_t taskgraph_degree,
		size_t taskgraph_height = 1
	);

	~scheduler();

	T *root();
	void spawn(T *t);
	void wait();

private:
	void create_workers(size_t taskgraph_degree, size_t taskgraph_height);

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
	size_t taskgraph_height
)
: m_nworkers(nworkers)
, m_state(state_t::terminated)
{
	m_state = state_t::initializing;

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	create_workers(taskgraph_degree, taskgraph_height);

	m_state = state_t::initialized;
}

template <typename T>
void scheduler<T>::create_workers(size_t taskgraph_degree, size_t taskgraph_height)
{
	using namespace internal;

	m_workers = new worker<T> *[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i] = new worker<T>(taskgraph_degree, taskgraph_height);

	for (int i = m_nworkers - 1; i >= 0; --i)
		m_workers[i]->async_init(i, m_nworkers, m_workers);
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

