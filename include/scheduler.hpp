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
		size_t taskgraph_degree,
		const topology &topo = topology(),
		size_t taskgraph_height = 1
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
	void create_worker(size_t id, size_t core_id, int victim_id, size_t flags);

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;
	const topology m_topology;

	size_t m_nworkers;
	worker_t *m_workers;
	internal::worker<T> *m_master;
};

template <typename T>
scheduler<T>::scheduler(
	size_t taskgraph_degree,
	const topology &topo,
	size_t taskgraph_height
)
: m_taskgraph_degree(internal::next_pow2(taskgraph_degree))
, m_taskgraph_height(taskgraph_height)
, m_topology(topo)
, m_nworkers(topo.get_nworkers())
{
	internal::Debug() << "Scheduler is working in debug mode";

	create_workers();
}

template <typename T>
void scheduler<T>::create_workers() {
	using namespace internal;

	m_workers = new worker_t[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].ready = false;

	auto &topo = m_topology.get();

	for (auto &elem : topo) {
		auto core = elem.first;
		auto w = elem.second;

		if (w.id == 0) {
			m_workers[0].thr = nullptr;
			create_worker(0, core, -1, w.flags);
			m_master = m_workers[0].wkr;
			continue;
		}

		auto v = topo.at(w.victim).id;
		m_workers[w.id].thr = new std::thread([=] {
			create_worker(w.id, core, v, w.flags);
			m_workers[w.id].wkr->steal_loop();
		});
	}
}

template <typename T>
void scheduler<T>::create_worker(size_t id, size_t core_id, int victim_id, size_t flags)
{
	using namespace internal;

	Debug() 
		<< "Init worker #" << id 
		<< " at CPU" << core_id
		<< " victim #" << victim_id
		<< " flags: " << flags;

	auto alloc = new lifo_allocator(predict_page_size());

	auto wkr = alloc->alloc<worker<T>>();
	new(wkr)
		worker<T>(id, alloc, m_taskgraph_degree, m_taskgraph_height, flags);

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

#if STACCATO_DEBUG
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

