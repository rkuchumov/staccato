#ifndef STACCATO_SCEDULER_H
#define STACCATO_SCEDULER_H

#include <cstdlib>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>

#include "utils.hpp"
#include "debug.hpp"

#include "worker.hpp"
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

	int get_worker_core(size_t id);

	void create_workers();
	void create_worker(size_t id);

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	size_t m_nworkers;
	worker_t *m_workers;
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
{
	using namespace internal;
	internal::Debug() << "Scheduler is working in debug mode";

	if (m_nworkers == 0)
		m_nworkers = std::thread::hardware_concurrency();

	worker<T>::m_power_of_width[0] = 1;
	worker<T>::m_max_power_id = 63;
	for (int i = 1; i < 64; ++i) {
		worker<T>::m_power_of_width[i] = worker<T>::m_power_of_width[i-1] * taskgraph_degree;
		if (worker<T>::m_power_of_width[i] > worker<T>::m_power_of_width[i-1])
			continue;
		worker<T>::m_max_power_id = i - 1;
		break;
	}

	for (int i = 0; i < 64; ++i) {
		internal::Debug() << i << ": " << worker<T>::m_power_of_width[i];
	}

	internal::Debug() << "max power id: " << worker<T>::m_max_power_id;

	create_workers();
}

template <typename T>
void scheduler<T>::create_workers()
{
	using namespace internal;

	m_workers = new worker_t[m_nworkers];
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].ready = false;

	create_worker(0);
	m_workers[0].thr = nullptr;
	m_master = m_workers[0].wkr;

	for (size_t i = 1; i < m_nworkers; ++i) {
		m_workers[i].thr = new std::thread([=] {
			create_worker(i);
			m_workers[i].wkr->steal_loop();
		});
	}

	for (size_t i = 0; i < m_nworkers; ++i)
		while (!m_workers[i].ready)
			std::this_thread::yield();

	for (size_t i = 0; i < m_nworkers; ++i) {
		for (size_t j = 0; j < m_nworkers; ++j) {
			if (i == j)
				continue;
			m_workers[i].wkr->cache_victim(m_workers[j].wkr);
		}
	}
}

template <typename T>
void scheduler<T>::create_worker(size_t id)
{
	using namespace internal;

	Debug() << "Init worker #" << id;

	auto alloc = new lifo_allocator(predict_page_size());

	int core_id = get_worker_core(id);

	auto wkr = alloc->alloc<worker<T>>();
	new(wkr)
		worker<T>(id, core_id, alloc, m_nworkers, m_taskgraph_degree, m_taskgraph_height);

	m_workers[id].alloc = alloc;
	m_workers[id].wkr = wkr;
	m_workers[id].ready = true;
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
int scheduler<T>::get_worker_core(size_t id)
{
	using namespace internal;

	static int affinity = -1;
	static int nsockets = -1;
	static int nthreads = -1;
	static int ncores = -1;

	if (affinity < 0) {
		affinity = read_affinity();
		Debug() << "affinity: " << affinity;
	}
	if (nsockets < 0) {
		nsockets = read_env_to_int("STACCATO_NSOCKETS");
		Debug() << "nsockets: " << nsockets;
	}
	if (nthreads < 0) {
		nthreads = read_env_to_int("STACCATO_NTHREADS");
		Debug() << "nthreads: " << nthreads;
	}
	if (ncores < 0) {
		ncores = read_env_to_int("STACCATO_NCORES");
		Debug() << "ncores: " << ncores;
	}

	if (!affinity || !nthreads || !ncores || !nsockets)
		return -1;

	int core = -1;

	if (affinity == 1) {
		core = id; 
	}

	if (affinity == 2) {
		core = ncores * nsockets * (id % nthreads) + id / nthreads; 
	}

	if (core >= 0)
		Debug() << "Worker #" << id << " at core " << core;

	return core;
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
void scheduler<T>::spawn(T *root)
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

