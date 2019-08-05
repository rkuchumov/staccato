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

	void print_load(uint64_t l);
	size_t max_power(uint64_t l);

	void update();

	bool balance();

	uint64_t top_task_load(uint64_t load) ;

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

	enum even_e {
		ITERATION = 0,
		MIGRATION_NO_MIN,
		MIGRATION_NO_MAX,
		MIGRATION_FAIL_EMPTY,
		MIGRATION_FAIL_RACE,
		MIGRATION_SUCCESS,
		MIGRATION_LOW_IMB,

		NR_EVENTS
	};

	size_t m_events[NR_EVENTS];
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


#if STACCATO_DEBUG
	load_samples.push_back(l);
#endif

	for (size_t i = 1; i < m_nworkers; ++i) {
		uint64_t l = m_workers[i].w->get_load();
		m_workers[i].load = l;
		m_workers[i].mailbox_size = m_workers[i].mailbox->size();

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

	for (size_t i = 1; i < worker<T>::m_max_power_id; ++i)
		if (worker<T>::m_power_of_width[i] > load)
			return worker<T>::m_power_of_width[i-1];

	return worker<T>::m_power_of_width[worker<T>::m_max_power_id-1];
}

template <typename T>
bool dispatcher<T>::balance()
{
	// return false;
	// if (m_events[ITERATION] > 100)  
		// return false;

	uint64_t load_min = 0;
	uint64_t load_max = 0;
	size_t load_argmin = 0;
	size_t load_argmax = 0;
	bool found_min = false;
	bool found_max = false;

	for (size_t i = 1; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		if (w->mailbox_size >= w->mailbox->capacity())
			continue;

		if (!found_min || w->load < load_min) {
			load_min = w->load;
			load_argmin = i;
			found_min = true;
		}
	}

	if (!found_min) {
		m_events[MIGRATION_NO_MIN]++;
		return false;
	}

	for (size_t i = 1; i < m_nworkers; ++i) {
		worker_state *w = m_workers + i;

		uint64_t l_top = top_task_load(w->load);
		STACCATO_ASSERT(w->load >= l_top, "inconsistent powers");
		uint64_t l_bottom = w->load - l_top;

		if (l_bottom <= load_min + l_top)
			continue;

		if (!found_max || w->load > load_max) {
			load_max = w->load;
			load_argmax = i;
			found_max = true;
		}
	}

	if (!found_max) {
		m_events[MIGRATION_NO_MAX]++;
		return false;
	}

	if (load_argmax == load_argmin)
		return false;

	// fprintf(stderr, "#%-5lu", m_events[ITERATION]);
	// fprintf(stderr, "%3zu ", load_argmax);
	// fprintf(stderr, "%3zu ", load_argmin);

	bool moved = try_move_task(load_argmax, load_argmin);

	// if (!moved)
	// 	fprintf(stderr, "FAIL ");
	// print_load(load_max);
	// fprintf(stderr, " -> ");
	// print_load(load_min);
	// fprintf(stderr, "\n");
	return moved;
}

template <typename T>
void dispatcher<T>::loop()
{
	while (!load_relaxed(m_stopped)) {
		m_events[ITERATION]++;
		while (true) {
			update();

			if (balance()) {
				m_events[MIGRATION_SUCCESS]++;
			} else {
				break;
			}
		}

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
		// std::this_thread::yield();
	}
}

template <typename T>
task<T> *dispatcher<T>::steal_task(task_deque<T> *head)
{
	task_deque<T> *tail = head;

	bool was_empty = false;
	auto t = tail->steal(&was_empty);

	if (t)
		return t;

	if (!was_empty)
		m_events[MIGRATION_FAIL_RACE]++;
	else
		m_events[MIGRATION_FAIL_EMPTY]++;

	return nullptr;
}

template <typename T>
bool dispatcher<T>::try_move_task(size_t w_from, size_t w_to)
{
	task<T> *t = steal_task(m_workers[w_from].head);
	if (!t)
		return false;

	m_workers[w_from].w->uncount_task(t->level());

	m_workers[w_to].w->count_task(t->level());

	m_workers[w_to].mailbox->put(reinterpret_cast<T *>(t));

	// fprintf(stderr, "[%2u] ", t->level());

	return true;
}

template <typename T>
size_t dispatcher<T>::max_power(uint64_t load)
{
	for (size_t i = worker<T>::m_max_power_id; i != 0; i--) {
		if (!load)
			break;

		unsigned n = 0;
		while (load >= worker<T>::m_power_of_width[i]) {
			load -= worker<T>::m_power_of_width[i];
			n++;
		}

		if (n)
			return i;
	}
	return 0;
}

template <typename T>
void dispatcher<T>::print_load(uint64_t load)
{
	for (size_t i = worker<T>::m_max_power_id; i != 0; i--) {
		if (!load)
			break;

		unsigned n = 0;
		while (load >= worker<T>::m_power_of_width[i]) {
			load -= worker<T>::m_power_of_width[i];
			n++;
		}

		if (n)
			fprintf(stderr, "%ux%lu + ", n, worker<T>::m_max_power_id-i);
	}
}


#if STACCATO_DEBUG
template <typename T>
void dispatcher<T>::print_stats()
{
	Debug() << "Dispatcher: ";
	Debug() << "   iterations:     " << m_events[ITERATION];
	Debug() << "   migrations:     " << m_events[MIGRATION_SUCCESS];
	Debug() << "   failed no min:  " << m_events[MIGRATION_NO_MIN];
	Debug() << "   failed no max:  " << m_events[MIGRATION_NO_MAX];
	Debug() << "   failed empty:   " << m_events[MIGRATION_FAIL_EMPTY];
	Debug() << "   failed race:    " << m_events[MIGRATION_FAIL_RACE];
	Debug() << "   low imbalance:  " << m_events[MIGRATION_LOW_IMB];

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
