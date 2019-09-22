#ifndef DISPATCHER_HPP_KVIXMHLF
#define DISPATCHER_HPP_KVIXMHLF

#include <cstdlib>
#include <thread>
#include <limits>
#include <iostream>
#include <chrono>

#if STACCATO_DEBUG
#include <fstream>
#endif

#include <pthread.h>

#include "utils.hpp"

#include "task_deque.hpp"
#include "lifo_allocator.hpp"
#include "task.hpp"
#include "counter.hpp"
#include "worker.hpp"

namespace staccato
{
namespace internal
{

template <typename T>
class dispatcher
{
public:
	dispatcher(
		size_t id,
		int core_id,
		size_t nnodes,
		lifo_allocator *alloc,
		size_t nworkers,
		size_t taskgraph_degree
	);

	~dispatcher();

	void register_worker(worker<T> *w);

	void stop();

	void loop();

	void print_stats();

private:
	struct worker_state {
		worker<T> *w;
		size_t node_id;
		task_deque<T> *head;
		const T *mailed_task;
		uint64_t load_deque;
	};

	void print_load(uint64_t l);
	size_t min_power(uint64_t l);

	void update();

	bool balance(size_t node);

	bool balance_between_nodes();

	uint64_t top_task_load(uint64_t load) ;

	task<T> *steal_task(task_deque<T> *head, size_t max_level);

	bool try_move_task(size_t w_from, size_t w_to, size_t max_level);

#if STACCATO_DEBUG
	std::vector<uint64_t> load_samples;
#endif

	const size_t m_id;
	const size_t m_nnodes;

	lifo_allocator *m_allocator;

	std::atomic_bool m_stopped;

	std::atomic_size_t m_nworkers;

	worker_state *m_workers;

	const size_t m_taskgraph_degree;

	enum even_e {
		ITERATION = 0,
		MIGRATION_NO_MIN,
		MIGRATION_NO_MAX,
		MIGRATION_FAIL_EMPTY,
		MIGRATION_FAIL_RACE,
		MIGRATION_SUCCESS,
		MIGRATION_SUCCESS_0,
		MIGRATION_SUCCESS_1,
		MIGRATION_SUCCESS_NODES,
		MIGRATION_LOW_IMB,
		MIGRATION_LOW_LEVEL,
		ZERO_LOAD,

		NR_EVENTS
	};

	size_t m_events[NR_EVENTS];
};

template <typename T>
dispatcher<T>::dispatcher(
	size_t id,
	int core_id,
	size_t nnodes,
	lifo_allocator *alloc,
	size_t nworkers,
	size_t taskgraph_degree
)
: m_id(id)
, m_nnodes(nnodes)
, m_allocator(alloc)
, m_stopped(false)
, m_nworkers(0)
, m_workers(nullptr)
, m_taskgraph_degree(taskgraph_degree)
{
	memset(m_events, 0, sizeof(m_events));

	m_workers = m_allocator->alloc_array<worker_state>(nworkers);

	if (core_id >= 0) {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core_id, &cpuset);

		pthread_t current_thread = pthread_self();    
		pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	}
}

template <typename T>
dispatcher<T>::~dispatcher()
{
	delete m_allocator;
}

template <typename T>
void dispatcher<T>::register_worker(worker<T> *w)
{
	m_workers[m_nworkers].w = w;
	m_workers[m_nworkers].node_id = w->node_id();
	m_workers[m_nworkers].head = w->get_deque();
	m_workers[m_nworkers].load_deque = 0;
	m_nworkers++;
}

template <typename T>
void dispatcher<T>::stop()
{
	m_stopped = true;
}

template <typename T>
void dispatcher<T>::update()
{
	uint64_t l = m_workers[0].w->get_deque_load();
	m_workers[0].load_deque = l;
	m_workers[0].mailed_task = m_workers[0].w->get_mailed_task();


	if (l == 0)
		m_events[ZERO_LOAD]++;
#if STACCATO_DEBUG
	load_samples.push_back(l);
#endif

	for (size_t i = 1; i < m_nworkers; ++i) {
		uint64_t l = m_workers[i].w->get_deque_load();
		m_workers[i].load_deque = l;
		m_workers[i].mailed_task = m_workers[i].w->get_mailed_task();

		if (l == 0)
			m_events[ZERO_LOAD]++;

#if STACCATO_DEBUG
		load_samples.push_back(l);
#endif
	}
}

template <typename T>
uint64_t dispatcher<T>::top_task_load(uint64_t load)
{
	if (load == 0)
		return 0;

	for (size_t i = 1; i < worker<T>::m_d_max; ++i)
		if (worker<T>::m_n[i] > load)
			return worker<T>::m_n[i-1];

	return worker<T>::m_n[worker<T>::m_d_max-1];
}

template <typename T>
bool dispatcher<T>::balance_between_nodes()
{
	uint64_t load_min = 0;
	uint64_t load_max_0 = 0;
	uint64_t load_max_0_top = 0;
	size_t load_argmin = 0;
	size_t load_argmax_0 = 0;
	bool found_min = false;
	bool found_max_0 = false;

	uint64_t load_0 = 0;
	uint64_t load_1 = 0;

	for (size_t i = 0; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		auto load = w->load_deque;

		if (w->node_id == 0) {
			load_0 += load;

			uint64_t lt = top_task_load(load);

			if (load != lt) {
				if (!found_max_0 || lt > load_max_0_top) {
					load_max_0_top = lt;
					load_max_0 = load;
					load_argmax_0 = i;
					found_max_0 = true;
				}
			}
		}

		if (w->node_id == 1) {
			load_1 += load;
		}
	}

	if (!found_max_0)
		return false;

	if (load_max_0_top == load_max_0)
		return false;

	if (load_max_0_top >= load_0)
		return false;

	if (load_1 + load_max_0_top >= load_0 - load_max_0_top)
		return false;

	for (size_t i = 0; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		if (w->node_id == 0)
			continue;

		if (w->mailed_task)
			continue;

		auto load = w->load_deque;

		if (!found_min || load < load_min) {
			load_min = load;
			load_argmin = i;
			found_min = true;
		}
	}

	if (!found_min)
		return false;

	fprintf(stderr, "#%-5lu", m_events[ITERATION]);

	fprintf(stderr, ".. %3zu %3zu ", load_argmax_0, load_argmin);

	bool moved = false;
	moved = try_move_task(load_argmax_0, load_argmin, min_power(load_max_0));
	fprintf(stderr, "[%2zd] ", min_power(load_max_0));
	if (!moved) fprintf(stderr, "FAIL ");
	print_load(m_workers[load_argmax_0].load_deque);
	fprintf(stderr, " | ");
	fprintf(stderr, " -> ");
	print_load(m_workers[load_argmin].load_deque);
	fprintf(stderr, "\n");

	return moved;
}

template <typename T>
bool dispatcher<T>::balance(size_t node)
{
	uint64_t load_min = 0;
	uint64_t load_max = 0;
	size_t load_argmin = 0;
	size_t load_argmax = 0;
	bool found_min = false;
	bool found_max = false;

	for (size_t i = 0; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		if (w->node_id != node)
			continue;

		auto load = w->load_deque;

		if (top_task_load(load) == load)
			continue;

		if (!found_max || load > load_max) {
			load_max = load;
			load_argmax = i;
			found_max = true;
		}
	}

	for (size_t i = 0; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		if (w->node_id != node)
			continue;

		if (w->mailed_task)
			continue;

		uint64_t l_top = top_task_load(load_max);

		auto load = w->load_deque;

		if (load + l_top >= load_max)
			continue;

		if (!found_min || load < load_min) {
			load_min = load;
			load_argmin = i;
			found_min = true;
		}
	}

	if (!found_min) {
		m_events[MIGRATION_NO_MIN]++;
		return false;
	}

	if (!found_max) {
		m_events[MIGRATION_NO_MAX]++;
		return false;
	}

	if (load_argmax == load_argmin)
		return false;

	fprintf(stderr, "#%-5lu", m_events[ITERATION]);
	fprintf(stderr, "N%zd ", node);
	fprintf(stderr, "%3zu ", load_argmax);
	fprintf(stderr, "%3zu ", load_argmin);

	fprintf(stderr, "[%2zd] ", min_power(load_max));
	bool moved = try_move_task(load_argmax, load_argmin, min_power(load_max));

	if (!moved)
		fprintf(stderr, "FAIL ");
	print_load(m_workers[load_argmax].load_deque);
	fprintf(stderr, " | ");
	fprintf(stderr, " -> ");
	print_load(m_workers[load_argmin].load_deque);
	fprintf(stderr, "\n");
	return moved;
}

template <typename T>
void dispatcher<T>::loop()
{
	while (!load_relaxed(m_stopped)) {
		m_events[ITERATION]++;

		while (m_nnodes) {
			update();
			if (balance_between_nodes())
				m_events[MIGRATION_SUCCESS_NODES]++;
			else
				break;
		}

		while (true) {
			update();

			bool b0 = false;
			bool b1 = false;

			if (balance(0)) {
				m_events[MIGRATION_SUCCESS_0]++;
				b0 = true;
			}

			if (m_nnodes) {
				if (balance(1)) {
					m_events[MIGRATION_SUCCESS_1]++;
					b1 = true;
				}
			}

			if (b0 || b1)
				continue;
			else
				break;
		}

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
		// std::this_thread::yield();
	}
}

template <typename T>
task<T> *dispatcher<T>::steal_task(task_deque<T> *head, size_t max_level)
{
	auto tail = head;
	size_t now_stolen = 0;
	size_t d = 0;

	while (true) {
		if (now_stolen >= m_taskgraph_degree - 1) {
			if (tail->get_next()) {
				tail = tail->get_next();
				now_stolen = 0;
				d++;
			}
		}

		if (tail->level() > max_level) {
			m_events[MIGRATION_LOW_LEVEL]++;
			return nullptr;
		}

		bool was_empty = false;
		auto t = tail->steal(&was_empty);

		if (!was_empty)
			m_events[MIGRATION_FAIL_RACE]++;
		else
			m_events[MIGRATION_FAIL_EMPTY]++;

		if (t)
			return t;

		if (!was_empty) {
			now_stolen++;
			continue;
		}

		if (!tail->get_next())
			return nullptr;

		tail = tail->get_next();
		now_stolen = 0;
		d++;
	}
}

template <typename T>
bool dispatcher<T>::try_move_task(size_t w_from, size_t w_to, size_t max_level)
{
	task<T> *t = steal_task(m_workers[w_from].head, max_level);
	if (!t)
		return false;

	t->worker()->uncount_task_deque(t->level());

	m_workers[w_to].w->mail_task(reinterpret_cast<T *>(t));

	return true;
}

template <typename T>
size_t dispatcher<T>::min_power(uint64_t load)
{
	for (size_t i = worker<T>::m_d_max; i != 0; i--) {
		if (!load)
			break;

		unsigned n = 0;
		while (load >= worker<T>::m_n[i]) {
			load -= worker<T>::m_n[i];
			n++;
		}

		if (n)
			return worker<T>::m_d_max-i;
	}
	return 0;
}

template <typename T>
void dispatcher<T>::print_load(uint64_t load)
{
	for (size_t i = worker<T>::m_d_max; i != 0; i--) {
		unsigned n = 0;
		while (load >= worker<T>::m_n[i]) {
			load -= worker<T>::m_n[i];
			n++;
		}

		fprintf(stderr, "%u", n);
	}
}


#if STACCATO_DEBUG
template <typename T>
void dispatcher<T>::print_stats()
{
	Debug() << "Dispatcher: ";
	Debug() << "   iterations:       " << m_events[ITERATION];
	Debug() << "   migrations nodes: " << m_events[MIGRATION_SUCCESS_NODES];
	Debug() << "   migrations 0:     " << m_events[MIGRATION_SUCCESS_0];
	Debug() << "   migrations 1:     " << m_events[MIGRATION_SUCCESS_1];
	Debug() << "   migrations:       " << m_events[MIGRATION_SUCCESS];
	Debug() << "   failed no min:    " << m_events[MIGRATION_NO_MIN];
	Debug() << "   failed no max:    " << m_events[MIGRATION_NO_MAX];
	Debug() << "   failed empty:     " << m_events[MIGRATION_FAIL_EMPTY];
	Debug() << "   failed race:      " << m_events[MIGRATION_FAIL_RACE];
	Debug() << "   low imbalance:    " << m_events[MIGRATION_LOW_IMB];
	Debug() << "   low level:        " << m_events[MIGRATION_LOW_LEVEL];
	Debug() << "   zero load:        " << m_events[ZERO_LOAD];

	std::ofstream out("load_samples.csv");
	size_t rows = load_samples.size() / m_nworkers;

	out << "i";
	for (size_t w = 0; w < m_nworkers; ++w)
		out << ",w" << w;
	out << "\n";

	size_t i = 0;
	for (size_t r = 0; r < rows; ++r) {
		out << r;
		for (size_t w = 0; w < m_nworkers; ++w) {
			out << "," << load_samples[i];
			i++;
		}
		out << "\n";
	}
}
#endif

}
}

#endif /* end of include guard: DISPATCHER_HPP_KVIXMHLF */
