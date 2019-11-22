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

		uint64_t load_deque;
		uint64_t load_mailbox;
		bool mailable;
		bool assigned;
	};

	bool balance(size_t node);

	task<T> *steal_task(task_deque<T> *head);

#if STACCATO_DEBUG
	std::vector<uint64_t> load_samples;
#endif

	const size_t m_nnodes;

	lifo_allocator *m_allocator;

	std::atomic_bool m_stopped;

	std::atomic_size_t m_nworkers;

	worker_state *m_workers;

	const size_t m_taskgraph_degree;

	// enum even_e {
	// 	ITERATION = 0,
	// 	MIGRATION_NO_MIN_0,
	// 	MIGRATION_NO_MAX_0,
	// 	MIGRATION_NO_MIN_1,
	// 	MIGRATION_NO_MAX_1,
	// 	MIGRATION_FAIL_EMPTY,
	// 	MIGRATION_FAIL_RACE,
	// 	MIGRATION_SUCCESS,
	// 	MIGRATION_SUCCESS_0,
	// 	MIGRATION_SUCCESS_1,
	// 	MIGRATION_SUCCESS_NODES,
	// 	MIGRATION_LOW_IMB,
	// 	MIGRATION_LOW_LEVEL,
	// 	DBG_0,
	// 	DBG_1,
	// 	DBG_2,
	// 	DBG_3,
	// 	ZERO_LOAD,
    //
	// 	NR_EVENTS
	// };
    //
	// size_t m_events[NR_EVENTS];
};

template <typename T>
dispatcher<T>::dispatcher(
	size_t nnodes,
	lifo_allocator *alloc,
	size_t nworkers,
	size_t taskgraph_degree
)
: m_nnodes(nnodes)
, m_allocator(alloc)
, m_stopped(false)
, m_nworkers(0)
, m_workers(nullptr)
, m_taskgraph_degree(taskgraph_degree)
{
	// memset(m_events, 0, sizeof(m_events));

	m_workers = m_allocator->alloc_array<worker_state>(nworkers);
	for (size_t i = 0; i < m_nworkers; ++i)
		m_workers[i].assigned = false;
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
void dispatcher<T>::loop()
{
	STACCATO_ASSERT(m_taskgraph_degree > 1, "Invalid task grath");
	STACCATO_ASSERT(m_nnodes > 1, "Can not balance between a single node");

	size_t per_node = (m_taskgraph_degree < m_nnodes) ? 1 : m_taskgraph_degree / m_nnodes;
	size_t left = m_taskgraph_degree;
	size_t node = m_nnodes - 1;

	Debug() << "tasks per node: " << per_node;

	m_workers[0].assigned = true;
	while (!load_relaxed(m_stopped)) {

		task<T> *t = steal_task(m_workers[0].head);
		if (!t) {
			std::this_thread::yield();
			continue;
		}

		size_t w_id = 0;

		bool found = false;
		for (size_t i = 1; i < m_nworkers; ++i) {
			worker_state *w = m_workers + i;

			if (w->node_id != node)
				continue;

			if (w->assigned)
				continue;

			w_id = i;
			found = true;
			break;
		}
		
		if (!found)
			STACCATO_ASSERT(found, "No worker found in the next node");

		m_workers[w_id].w->mail_task(reinterpret_cast<T *>(t));
		m_workers[w_id].assigned = true;

		left--;

		Debug() << "assigned task to w" << w_id << " at node "  << node << " left: " << left;

		size_t left_in_node = left % per_node;
		if (left_in_node == 0) {
			STACCATO_ASSERT(node != 0, "incosistend node ids");

			for (size_t i = 0; i < m_nworkers; ++i) {
				if (m_workers[i].node_id == node)
					m_workers[i].w->allow_stealing();
			}

			node--;
		}

		if (left == 1) {
			for (size_t i = 0; i < m_nworkers; ++i) {
				if (m_workers[i].node_id == 0)
					m_workers[i].w->allow_stealing();
			}
			Debug() << "Dispatcher exit\n";
			return;
		}
	}

	Debug() << "Dispatcher exit\n";
}

template <typename T>
task<T> *dispatcher<T>::steal_task(task_deque<T> *head)
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

		if (tail->level() > 1) {
			// m_events[MIGRATION_LOW_LEVEL]++;
			return nullptr;
		}

		bool was_empty = false;
		auto t = tail->steal(&was_empty);

		// if (!was_empty)
		// 	m_events[MIGRATION_FAIL_RACE]++;
		// else
		// 	m_events[MIGRATION_FAIL_EMPTY]++;

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

#if STACCATO_DEBUG
template <typename T>
void dispatcher<T>::print_stats()
{
	// Debug() << "Dispatcher: ";
	// Debug() << "   iterations:       " << m_events[ITERATION];
	// Debug() << "   migrations nodes: " << m_events[MIGRATION_SUCCESS_NODES];
	// Debug() << "   migrations 0:     " << m_events[MIGRATION_SUCCESS_0];
	// Debug() << "   migrations 1:     " << m_events[MIGRATION_SUCCESS_1];
	// Debug() << "   migrations:       " << m_events[MIGRATION_SUCCESS];
	// Debug() << "   failed no min 0:  " << m_events[MIGRATION_NO_MIN_0];
	// Debug() << "   failed no max 0:  " << m_events[MIGRATION_NO_MAX_0];
	// Debug() << "   failed no min 1:  " << m_events[MIGRATION_NO_MIN_1];
	// Debug() << "   failed no max 1:  " << m_events[MIGRATION_NO_MAX_1];
	// Debug() << "   failed empty:     " << m_events[MIGRATION_FAIL_EMPTY];
	// Debug() << "   failed race:      " << m_events[MIGRATION_FAIL_RACE];
	// Debug() << "   low imbalance:    " << m_events[MIGRATION_LOW_IMB];
	// Debug() << "   low level:        " << m_events[MIGRATION_LOW_LEVEL];
	// Debug() << "   zero load:        " << m_events[ZERO_LOAD];
	// Debug() << "   DBG_0:        " << m_events[DBG_0];
	// Debug() << "   DBG_1:        " << m_events[DBG_1];
	// Debug() << "   DBG_2:        " << m_events[DBG_2];
	// Debug() << "   DBG_3:        " << m_events[DBG_3];
    //
	// std::ofstream out("load_samples.csv");
	// size_t rows = load_samples.size() / m_nworkers;
    //
	// out << "i";
	// for (size_t w = 0; w < m_nworkers; ++w)
	// 	out << ",w" << w;
	// out << "\n";
    //
	// size_t i = 0;
	// for (size_t r = 0; r < rows; ++r) {
	// 	out << r;
	// 	for (size_t w = 0; w < m_nworkers; ++w) {
	// 		out << "," << load_samples[i];
	// 		i++;
	// 	}
	// 	out << "\n";
	// }
}
#endif

}
}

#endif /* end of include guard: DISPATCHER_HPP_KVIXMHLF */
