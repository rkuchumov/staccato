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


namespace staccato
{

template <typename T>
class task;

typedef std::vector<std::pair<size_t, int>> topology_t;

template <typename T>
class scheduler
{
public:

	scheduler(
		size_t nworkers,
		size_t taskgraph_degree,
		size_t taskgraph_height = 1,
		const topology_t &topology = {}
	);

	~scheduler();

	T *root();
	void spawn(T *t);
	void wait();

private:
	topology_t init_topology(
		size_t degree,
		const topology_t &topology
	);

	void create_workers(
		size_t taskgraph_degree,
		size_t taskgraph_height,
		const topology_t &topology);

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
	const topology_t &topology
)
: m_nworkers(nworkers)
, m_state(state_t::terminated)
{
	internal::Debug() << "Scheduler is working in debug mode";

	m_state = state_t::initializing;

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	auto topo = init_topology(taskgraph_degree, topology);

	create_workers(taskgraph_degree, taskgraph_height, topo);

	m_state = state_t::initialized;
}

template <typename T>
topology_t scheduler<T>::init_topology(
	size_t degree,
	const topology_t &topology)
{
	if (topology.size() == m_nworkers)
		return topology;
	else if (!topology.empty())
		internal::Debug()
			<< "The size supplied topology is not equal to the number of workers";

	topology_t topo;

	for (int i = 0; i < m_nworkers; ++i) {
		topo.push_back({i, i - 1});
	}

	// size_t nthreads = std::thread::hardware_concurrency();
	// topo.push_back({0, -1});
	// topo.push_back({nthreads / 2, 0});
    //
	// for (size_t i = 1; i < nthreads / 2; ++i) {
	// 	topo.push_back({i, nthreads / 2 + i - 1});
	// 	topo.push_back({nthreads / 2 + i, i});
	// }

	return topo;
}

template <typename T>
void scheduler<T>::create_workers(
	size_t taskgraph_degree,
	size_t taskgraph_height,
	const topology_t &topology
) {
	using namespace internal;

	auto degree = next_pow2(taskgraph_degree);
	m_workers = new worker<T> *[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i] = new worker<T>(degree, taskgraph_height);

	for (size_t i = 0; i < m_nworkers; ++i) {
		auto core = topology[i].first;
		auto victim = topology[i].second;
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

