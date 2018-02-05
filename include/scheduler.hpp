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
	// TODO: rename max_degree to something related to task tree
	scheduler(size_t nworkers, size_t max_degree);

	~scheduler();

	void spawn_and_wait(T *root);

// private:
// 	friend class task;
// 	friend class internal::worker;

	void create_workers();

	enum state_t { 
		terminated,
		initializing,
		initialized,
		terminating,
	};

	size_t m_nworkers;
	size_t m_max_degree;

	std::atomic<state_t> m_state;

	internal::worker<T> **m_workers;
};

template <typename T>
scheduler<T>::scheduler(size_t nworkers, size_t max_degree)
: m_nworkers(nworkers)
, m_max_degree(max_degree)
, m_state(state_t::terminated)
{
	m_state = state_t::initializing;

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	create_workers();

	m_state = state_t::initialized;
}

template <typename T>
void scheduler<T>::create_workers()
{
	using namespace internal;

	m_workers = new worker<T> *[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i] = new worker<T>();

	for (int i = m_nworkers - 1; i >= 0; --i)
		m_workers[i]->async_init(i, m_nworkers, m_workers, m_max_degree);
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
void scheduler<T>::spawn_and_wait(T *root)
{
	m_workers[0]->task_loop(root, root);
}

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

