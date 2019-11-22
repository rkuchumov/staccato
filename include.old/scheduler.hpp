#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>

#include <hwloc.h>

#include "utils.hpp"
#include "debug.hpp"

#include "worker.hpp"
#include "dispatcher.hpp"
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

	scheduler(
		size_t taskgraph_degree,
		size_t nworkers = 0,
		size_t taskgraph_height = 1
	);

	~scheduler();

	void spawn_and_wait(T *root);

private:
	struct worker_t {
		std::thread *thr;
		internal::lifo_allocator *alloc;
		internal::worker<T> * wkr;
		std::atomic_bool ready;
	};

	struct dispatcher_t {
		std::thread *thr;
		internal::lifo_allocator *alloc;
		internal::dispatcher<T> *dsp;
		std::atomic_bool ready;
	};

    inline size_t predict_page_size() const;

	void get_worker_location(size_t id, int &core, int &node, int &nvicims);

	void create_workers();
	void create_worker(size_t id, int core, int node, int nvicims);

	void create_dispatchers();
	void create_dispatcher(size_t id);

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	size_t m_nworkers;
	size_t m_ndispatchers;
	worker_t *m_workers;
	dispatcher_t *m_dispatchers;
	internal::worker<T> *m_master;
};

template <typename T>
scheduler<T>::scheduler(
	size_t taskgraph_degree,
	size_t nworkers,
	size_t taskgraph_height
)
: m_taskgraph_degree(internal::next_pow2(taskgraph_degree))
, m_taskgraph_height(taskgraph_height)
, m_nworkers(nworkers)
, m_ndispatchers(1)
{
	using namespace internal;
	internal::Debug() << "Scheduler is working in debug mode";

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	worker<T>::compute_powers(taskgraph_degree);

	create_workers();

	create_dispatchers();
}

template <typename T>
void scheduler<T>::create_workers()
{
	using namespace internal;

	hwloc_topology_t topology;
	int nbcores;

	hwloc_topology_init(&topology);  // initialization
	hwloc_topology_load(topology);   // actual detection

	nbcores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	printf("%d cores\n", nbcores);

	hwloc_topology_destroy(topology);

	m_workers = new worker_t[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].ready = false;

	int nvictims = 1;
	int core_id = -1;
	int node_id = -1;

	get_worker_location(0, core_id, node_id, nvictims);
	create_worker(0, core_id, node_id, nvictims);
	m_workers[0].thr = nullptr;
	m_master = m_workers[0].wkr;

	for (size_t i = 1; i < m_nworkers; ++i) {
		get_worker_location(i, core_id, node_id, nvictims);
		m_workers[i].thr = new std::thread([=] {
			create_worker(i, core_id, node_id, nvictims);
			m_workers[i].wkr->steal_loop();
		});
	}

	for (size_t i = 0; i < m_nworkers; ++i)
		while (!m_workers[i].ready)
			std::this_thread::yield();

	for (size_t i = 0; i < m_nworkers; ++i) {
		std::cerr << i << " (" << m_workers[i].wkr->node_id() << ") : ";
		for (size_t j = 0; j < m_nworkers; ++j) {
			if (i == j)
				continue;

			if (m_workers[i].wkr->node_id() == m_workers[j].wkr->node_id()) {
				std::cerr << j << " ";
				m_workers[i].wkr->cache_victim(m_workers[j].wkr);
			}
		}
		std::cerr << "\n";
	}

}

template <typename T>
void scheduler<T>::create_dispatchers()
{
	using namespace internal;

	m_dispatchers = new dispatcher_t[m_ndispatchers];
	for (size_t i = 0; i < m_ndispatchers; ++i)
		m_dispatchers[i].ready = false;

	for (size_t i = 0; i < m_ndispatchers; ++i) {
		m_dispatchers[i].thr = new std::thread([=] {
			create_dispatcher(i);
			m_dispatchers[i].dsp->loop();
		});
	}

	for (size_t i = 0; i < m_ndispatchers; ++i)
		while (!m_dispatchers[i].ready)
			std::this_thread::yield();
}

template <typename T>
void scheduler<T>::create_worker(size_t id, int core, int node, int nvictims)
{
	using namespace internal;

	// Debug() << "Init worker #" << id;

	auto alloc = new lifo_allocator(predict_page_size());

	auto wkr = alloc->alloc<worker<T>>();
	new(wkr) worker<T>(
		id,
		core,
		node,
		alloc,
		nvictims,
		m_taskgraph_degree,
		m_taskgraph_height
	);

	m_workers[id].alloc = alloc;
	m_workers[id].wkr = wkr;
	m_workers[id].ready = true;
}

template <typename T>
void scheduler<T>::create_dispatcher(size_t id)
{
	using namespace internal;

	Debug() << "Init dispatcher #" << id;

	auto alloc = new lifo_allocator(predict_page_size());

	int core_id = 41;
	size_t nnodes = 2;

	auto dsp = alloc->alloc<dispatcher<T>>();
	new (dsp) dispatcher<T>(
		id,
		core_id,
		nnodes,
		alloc,
		m_nworkers,
		m_taskgraph_degree
	);

	m_dispatchers[id].alloc = alloc;
	m_dispatchers[id].dsp = dsp;

	for (size_t w = 0; w < m_nworkers; ++w) {
		dsp->register_worker(m_workers[w].wkr);
	}

	m_dispatchers[id].ready = true;
}

/* TODO: use enums <09-12-18, yourname> */
int read_affinity() {
	const char* env = std::getenv("STACCATO_AFFINITY");

	if (!env)
		return 0;

	if (std::strcmp(env, "scatter") == 0)
		return 1;

	if (std::strcmp(env, "compact") == 0)
		return 2;

	return 0;
}

int read_env_to_int(const char *name) {
	const char* env = std::getenv(name);

	if (!env)
		return 0;

	return std::atoi(env);
}

template <typename T>
void scheduler<T>::get_worker_location(size_t id, int &core, int &node, int &nvictims)
{
	using namespace internal;

	// nvictims = 28;
	// node = 0;

	nvictims = 14;
	node = id % 2;

	core = 14 * (id % 2) + id / 2; 

	// nvictims = 28;
	// node = 0;
	// core = 14 * (id % 2) + id / 2; 

	// nvictims = 28;
	// node = 0;
	// core = -1;

	// nvictims = 14;
	// node = 0;
	// core = id; 

	Debug() << "Worker #" << id << "\t at node " << node << ", core " << core;
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
scheduler<T>::~scheduler()
{
	for (size_t i = 1; i < m_nworkers; ++i) {
		while (!m_workers[i].ready)
			std::this_thread::yield();

		m_workers[i].wkr->stop();
	}

	for (size_t i = 0; i < m_ndispatchers; ++i) {
		while (!m_dispatchers[i].ready)
			std::this_thread::yield();

		m_dispatchers[i].dsp->stop();
	}

#if STACCATO_DEBUG
	internal::counter::print_header();
	internal::counter totals;
	for (size_t i = 0; i < m_nworkers; ++i) {
		m_workers[i].wkr->get_counter().print_row(i);
		totals.join(m_workers[i].wkr->get_counter());
	}
	totals.print_totals();
	for (size_t i = 0; i < m_ndispatchers; ++i)
		m_dispatchers[i].dsp->print_stats();
#endif

	for (size_t i = 1; i < m_nworkers; ++i)
		m_workers[i].thr->join();

	for (size_t i = 0; i < m_ndispatchers; ++i)
		m_dispatchers[i].thr->join();

	for (size_t i = 0; i < m_nworkers; ++i) {
		delete m_workers[i].alloc;
		delete m_workers[i].thr;
	}

	for (size_t i = 0; i <m_ndispatchers; ++i) {
		delete m_dispatchers[i].alloc;
		delete m_dispatchers[i].thr;
	}

	delete []m_workers;
	delete []m_dispatchers;
}

template <typename T>
void scheduler<T>::spawn_and_wait(T *root)
{
	m_master->spawn_root(root);
}

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

