#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>

// TODO: check if present
#include <pthread.h>

#include "constants.hpp"
#include "utils.hpp"
#include "worker.hpp"
#include "topology.hpp"
#include "lifo_allocator.hpp"
#include "counter.hpp"

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
	struct worker_t {
		std::thread *thr;
		internal::lifo_allocator *alloc;
		internal::worker<T> * wkr;
		std::atomic_bool ready;
	};

    inline size_t predict_page_size() const;
	void pin_thread(size_t core_id);

	void create_workers();
	void create_worker(size_t id, size_t core_id, int victim_id);

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;
	const topology m_topology;

	size_t m_nworkers;
	worker_t *m_workers;
	internal::worker<T> *m_master;
};

template <typename T>
scheduler<T>::scheduler(
	size_t nworkers,
	size_t taskgraph_degree,
	size_t taskgraph_height,
	const topology &topo
)
: m_taskgraph_degree(internal::next_pow2(taskgraph_degree))
, m_taskgraph_height(taskgraph_height)
, m_topology(topo)
, m_nworkers(nworkers)
{
	internal::Debug() << "Scheduler is working in debug mode";

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	create_workers();
}

template <typename T>
void scheduler<T>::create_workers() {
	using namespace internal;

	m_workers = new worker_t[m_nworkers];
	for (int i = 0; i < m_nworkers; ++i)
		m_workers[i].ready = false;

	for (size_t i = 0; i < m_topology.get().size(); ++i) {
		if (i >= m_nworkers)
			break;

		auto w = m_topology.get()[i];

		if (i == 0) {
			m_workers[0].thr = nullptr;
			create_worker(0, w.core, w.victim);
			m_master = m_workers[0].wkr;
			continue;
		}

		m_workers[i].thr = new std::thread([=] {
			create_worker(i, w.core, w.victim);
			m_workers[i].wkr->steal_loop();
		});
	}
}

template <typename T>
void scheduler<T>::create_worker(size_t id, size_t core_id, int victim_id)
{
	using namespace internal;

	Debug() 
		<< "Init worker #" << id 
		<< " at CPU" << core_id
		<< " victim #" << victim_id;

	auto alloc = new lifo_allocator(predict_page_size());

	auto wkr = alloc->alloc<worker<T>>();
	new(wkr)
		worker<T>(id, alloc, m_taskgraph_degree, m_taskgraph_height);

	pin_thread(core_id);

	m_workers[id].alloc = alloc;
	m_workers[id].wkr = wkr;
	m_workers[id].ready = true;

	if (victim_id >= 0) {
		while (!m_workers[victim_id].ready)
			std::this_thread::yield();

		auto v = m_workers[victim_id].wkr;
		wkr->set_victim(v);
	}
}

template <typename T>
inline size_t scheduler<T>::predict_page_size() const
{
	using namespace internal;

	size_t s = 0;
	s += alignof(task_deque<T>) + sizeof(task_deque<T>);
	s += alignof(T) + sizeof(T) * m_taskgraph_degree;
	s *= m_taskgraph_height;
	return s;
}

template <typename T>
void scheduler<T>::pin_thread(size_t core_id)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	pthread_t current_thread = pthread_self();    
	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

template <typename T>
scheduler<T>::~scheduler()
{
	for (size_t i = 1; i < m_nworkers; ++i)
	{
		while (!m_workers[i].ready)
			std::this_thread::yield();

		m_workers[i].wkr->stop();
	}

#ifdef STACCATO_DEBUG
	internal::counter::print_header();
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].wkr->print_counters();
#endif

	for (size_t i = 1; i < m_nworkers; ++i)
		m_workers[i].thr->join();

	for (size_t i = 0; i < m_nworkers; ++i) {
		delete m_workers[i].alloc;
		delete m_workers[i].thr;
	}

	delete []m_workers;
}

template <typename T>
T *scheduler<T>::root()
{
	return m_master->root_allocate();
}

template <typename T>
void scheduler<T>::spawn(T *)
{
	m_master->root_commit();
}

template <typename T>
void scheduler<T>::wait()
{
	m_master->root_wait();
}

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

