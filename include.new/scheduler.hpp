#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>
#include <algorithm>

#include "utils.hpp"
#include "debug.hpp"

#include "worker.hpp"
#include "dispatcher.hpp"
#include "lifo_allocator.hpp"
#include "counter.hpp"

#include <hwloc.h>

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
		size_t taskgraph_height = 1,
		bool single_node = true
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

	void create_workers();
	void create_worker(size_t id, int core, int node, int nvicims);

	void create_dispatcher();

	void parse_topology();

	void parse_topology(
		hwloc_topology_t topology,
		hwloc_obj_t obj
	);

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	std::vector<std::vector<unsigned> > m_topology;

	size_t m_nworkers;
	worker_t *m_workers;
	dispatcher_t *m_dispatcher;
	internal::worker<T> *m_master;
	bool m_single_node;
};

template <typename T>
scheduler<T>::scheduler(
	size_t taskgraph_degree,
	size_t nworkers,
	size_t taskgraph_height,
	bool single_node
)
: m_taskgraph_degree(internal::next_pow2(taskgraph_degree))
, m_taskgraph_height(taskgraph_height)
, m_nworkers(nworkers)
, m_workers(nullptr)
, m_dispatcher(nullptr)
, m_master(nullptr)
, m_single_node(single_node)
{
	using namespace internal;
	internal::Debug() << "Scheduler is working in debug mode";

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	parse_topology();

	create_workers();

	if (m_topology.size() > 1) {
		create_dispatcher();
	} else {
		for (size_t i = 0; i < m_nworkers; ++i)
			m_workers[i].wkr->allow_stealing();
	}
}

template <typename T>
void scheduler<T>::parse_topology(
	hwloc_topology_t topology,
	hwloc_obj_t obj
) {

	if (obj->type == HWLOC_OBJ_NUMANODE)
		m_topology.push_back(std::vector<unsigned>());

	if (obj->type == HWLOC_OBJ_PU) {
		if (m_topology.empty())
			m_topology.push_back(std::vector<unsigned>());
		m_topology.back().push_back(obj->os_index);
	}

	for (unsigned i = 0; i < obj->arity; i++) {
		parse_topology(topology, obj->children[i]);
	}
}

template <typename T>
void scheduler<T>::parse_topology()
{
	using namespace internal;

	if (m_single_node) {
		m_topology.push_back(std::vector<unsigned>());
		for (size_t i = 0; i < std::thread::hardware_concurrency(); i++)
			m_topology[0].push_back(i);
		return;
	}

	hwloc_topology_t topology;

	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);

	parse_topology(topology, hwloc_get_root_obj(topology));

	hwloc_topology_destroy(topology);

	for (size_t n = 0; n < m_topology.size(); ++n) {
		std::sort(m_topology[n].begin(), m_topology[n].end());

		Debug() << "NUMA " << n << ", no. of cores: " << m_topology[n].size();
		// for (size_t c = 0; c < m_topology[n].size(); ++c)
		// 	std::cout << m_topology[n][c] << " ";
		// std::cout << "\n";
	}
}

template <typename T>
void scheduler<T>::create_workers()
{
	using namespace internal;

	m_workers = new worker_t[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].ready = false;

	bool is_master = true;

	for (size_t w = 0; w < m_nworkers; ++w) {
		size_t n = w % m_topology.size();
		size_t c = m_topology[n][w / m_topology.size()];
		size_t v = m_topology[n].size();

		if (is_master) {
			create_worker(w, c, n, v);
			m_workers[w].thr = nullptr;
			m_master = m_workers[w].wkr;
			is_master = false;
		} else {
			m_workers[w].thr = new std::thread([=] {
			create_worker(w, c, n, v);
				m_workers[w].wkr->steal_loop();
			});
		}

		Debug() << "Worker #" << w << "\t at node " << n << ", core " << c;
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
void scheduler<T>::create_dispatcher()
{
	using namespace internal;

	m_dispatcher = new dispatcher_t;
	m_dispatcher->ready = false;

	m_dispatcher->thr = new std::thread([=] {

		auto alloc = new lifo_allocator(predict_page_size());

		auto dsp = alloc->alloc<dispatcher<T>>();
		new (dsp) dispatcher<T>(
			m_topology.size(),
			alloc,
			m_nworkers,
			m_taskgraph_degree
		);

		m_dispatcher->alloc = alloc;
		m_dispatcher->dsp = dsp;

		for (size_t w = 0; w < m_nworkers; ++w) {
			dsp->register_worker(m_workers[w].wkr);
		}

		m_dispatcher->ready = true;

		m_dispatcher->dsp->loop();
	});

	while (!m_dispatcher->ready)
		std::this_thread::yield();
}

template <typename T>
void scheduler<T>::create_worker(size_t id, int core, int node, int nvictims)
{
	using namespace internal;

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

	if (m_dispatcher) {
		while (!m_dispatcher->ready)
			std::this_thread::yield();
		m_dispatcher->dsp->stop();
	}

#if STACCATO_DEBUG
	internal::counter::print_header();
	internal::counter totals;
	for (size_t i = 0; i < m_nworkers; ++i) {
		m_workers[i].wkr->get_counter().print_row(i);
		totals.join(m_workers[i].wkr->get_counter());
	}
	totals.print_totals();
	if (m_dispatcher)
		m_dispatcher->dsp->print_stats();
#endif

	for (size_t i = 1; i < m_nworkers; ++i)
		m_workers[i].thr->join();

	if (m_dispatcher)
		m_dispatcher->thr->join();

	for (size_t i = 0; i < m_nworkers; ++i) {
		delete m_workers[i].alloc;
		delete m_workers[i].thr;
	}

	if (m_dispatcher) {
		delete m_dispatcher->alloc;
		delete m_dispatcher->thr;
	}

	delete []m_workers;
	delete []m_dispatcher;
}

template <typename T>
void scheduler<T>::spawn_and_wait(T *root)
{
	m_master->spawn_root(root);
}

} /* namespace:staccato */ 

#endif /* end of include guard: STACCATO_SCEDULER_H */

