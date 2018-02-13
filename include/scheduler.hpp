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
#include "worker.hpp"


namespace staccato
{

template <typename T>
class task;

template <typename T>
class scheduler
{
public:
	typedef const std::vector<std::pair<size_t, int>> &affinity_t;

	scheduler(
		size_t nworkers,
		size_t taskgraph_degree,
		size_t taskgraph_height = 1,
		affinity_t &affinity_map = {}
	);

	~scheduler();

	T *root();
	void spawn(T *t);
	void wait();

private:
	void create_workers(
		size_t taskgraph_degree,
		size_t taskgraph_height,
		affinity_t affinity_map);

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
	affinity_t affinity_map
)
: m_nworkers(nworkers)
, m_state(state_t::terminated)
{
	m_state = state_t::initializing;

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	create_workers(taskgraph_degree, taskgraph_height, affinity_map);

	m_state = state_t::initialized;
}

template <typename T>
void scheduler<T>::create_workers(
	size_t taskgraph_degree,
	size_t taskgraph_height,
	affinity_t affinity_map
) {
	using namespace internal;

	m_workers = new worker<T> *[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i] = new worker<T>(taskgraph_degree, taskgraph_height);

	if (affinity_map.size() != m_nworkers) {
		m_workers[0]->async_init(true, 0, nullptr);
		for (size_t i = 1; i < m_nworkers; ++i)
			m_workers[i]->async_init(false, i, m_workers[i - 1]);
		return;
	} 

	for (size_t i = 0; i < m_nworkers; ++i) {
		auto victim = affinity_map[i].second;
		auto core = affinity_map[i].first;
		if (victim < 0)
			m_workers[i]->async_init(i == 0, core, nullptr);
		else
			m_workers[i]->async_init(i == 0, core, m_workers[victim]);
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

