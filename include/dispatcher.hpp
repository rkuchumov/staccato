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
		task_deque<T> *head;
		task_mailbox<T> *mailbox;
		uint64_t load;
		size_t mailbox_size;
	};

	uint64_t m_load_min;
	uint64_t m_load_max;
	size_t m_load_argmin;
	size_t m_load_argmax;

	void update();

	void balance();

	task<T> *steal_task(task_deque<T> *head);

	bool try_move_task(size_t w_from, size_t w_to);

#if STACCATO_DEBUG
	std::vector<uint64_t> load_samples;
#endif

	const size_t m_id;
	lifo_allocator *m_allocator;

	std::atomic_bool m_stopped;

	std::atomic_size_t m_nworkers;

	worker_state *m_workers;

	const size_t m_taskgraph_degree;
};

template <typename T>
dispatcher<T>::dispatcher(
	size_t id,
	int core_id,
	lifo_allocator *alloc,
	size_t nworkers,
	size_t taskgraph_degree
)
: m_id(id)
, m_allocator(alloc)
, m_stopped(false)
, m_nworkers(0)
, m_workers(nullptr)
, m_taskgraph_degree(taskgraph_degree)
{
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
	m_workers[m_nworkers].head = w->get_deque();
	m_workers[m_nworkers].mailbox = w->get_mailbox();;
	m_workers[m_nworkers].load = 0;
	m_workers[m_nworkers].mailbox_size = 0;
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
	uint64_t l = m_workers[0].w->get_load();
	m_workers[0].load = l;
	m_workers[0].mailbox_size = m_workers[0].mailbox->size();

	m_load_min = l;
	m_load_max = l;
	m_load_argmin = 0;
	m_load_argmax = 0;

#if STACCATO_DEBUG
	load_samples.push_back(l);
#endif

	for (size_t i = 1; i < m_nworkers; ++i) {
		uint64_t l = m_workers[i].w->get_load();
		m_workers[i].load = l;
		m_workers[i].mailbox_size = m_workers[i].mailbox->size();

		if (l > m_load_max) {
			m_load_max = l;
			m_load_argmax = i;
		}

		if (l < m_load_min) {
			m_load_min = l;
			m_load_argmin = i;
		}

#if STACCATO_DEBUG
		load_samples.push_back(l);
#endif
	}

	if (m_load_argmin != m_load_argmax) {
		bool ok = try_move_task(m_load_argmax, m_load_argmin);
		if (ok) {
			std::clog << "moved ";
			std::clog << m_load_argmax << ":" << m_load_max << " -> ";
			std::clog << m_load_argmin << ":" << m_load_min << "\n";
		} else {
			std::clog << "move failed\n";
		}
	}

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);
}

template <typename T>
void dispatcher<T>::loop()
{
	while (!load_relaxed(m_stopped)) {
		update();

		std::this_thread::yield();
	}
}

template <typename T>
task<T> *dispatcher<T>::steal_task(task_deque<T> *head)
{
	task_deque<T> *tail = head;
	size_t now_stolen = 0;

	while (true) {
		if (now_stolen >= m_taskgraph_degree - 1) {
			if (tail->get_next()) {
				tail = tail->get_next();
				now_stolen = 0;
			}
		}

		bool was_empty = false;
		auto t = tail->steal(&was_empty);

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
	}
}

template <typename T>
bool dispatcher<T>::try_move_task(size_t w_from, size_t w_to)
{
	task<T> *t = steal_task(m_workers[w_from].head);
	if (!t)
		return false;


	m_workers[w_to].mailbox->put(reinterpret_cast<T *>(t));

	return true;
}

#if STACCATO_DEBUG
template <typename T>
void dispatcher<T>::print_stats()
{
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
