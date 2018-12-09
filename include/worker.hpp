#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>
#include <limits>

#include <pthread.h>

#include "utils.hpp"

#include "task_deque.hpp"
#include "lifo_allocator.hpp"
#include "task.hpp"
#include "counter.hpp"

namespace staccato
{
namespace internal
{

template <typename T>
class worker
{
public:
	worker(
		size_t id,
		int core_id,
		lifo_allocator *alloc,
		size_t nvictims,
		size_t taskgraph_degree,
		size_t taskgraph_height
	);

	~worker();

	void cache_victim(worker<T> *victim);

	void stop();

	void local_loop(task_deque<T> *tail);

	void steal_loop();

	T *root_allocate();
	void root_commit();
	void root_wait();

#if STACCATO_DEBUG
	void print_counters();
#endif

private:
	void init(size_t core_id, worker<T> *victim);

	void grow_tail(task_deque<T> *tail);

	task_deque<T> *get_victim();

	task<T> *steal_task(task_deque<T> *tail, task_deque<T> **victim);

	const size_t m_id;
	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;
	lifo_allocator *m_allocator;

#if STACCATO_DEBUG
	counter m_counter;
#endif

	std::atomic_bool m_stopped;

	std::atomic_size_t m_nvictims;

	task_deque<T> **m_victims_heads;

	task_deque<T> *m_head_deque;
};

template <typename T>
worker<T>::worker(
	size_t id,
	int core_id,
	lifo_allocator *alloc,
	size_t nvictims,
	size_t taskgraph_degree,
	size_t taskgraph_height
)
: m_id(id)
, m_taskgraph_degree(taskgraph_degree)
, m_taskgraph_height(taskgraph_height)
, m_allocator(alloc)
, m_stopped(false)
, m_nvictims(0)
, m_victims_heads(nullptr)
, m_head_deque(nullptr)
{
	m_victims_heads = m_allocator->alloc_array<task_deque<T> *>(nvictims);

	auto d = m_allocator->alloc<task_deque<T>>();
	auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
	new(d) task_deque<T>(m_taskgraph_degree, t);

	m_head_deque = d;

	for (size_t i = 1; i < m_taskgraph_height + 1; ++i) {
		auto n = m_allocator->alloc<task_deque<T>>();
		auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
		new(n) task_deque<T>(m_taskgraph_degree, t);

		d->set_next(n);
		d = n;
	}

	if (core_id >= 0) {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core_id, &cpuset);

		pthread_t current_thread = pthread_self();    
		pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	}
}

template <typename T>
worker<T>::~worker()
{
	delete m_allocator;
}

template <typename T>
void worker<T>::cache_victim(worker<T> *victim)
{
	m_victims_heads[m_nvictims] = victim->m_head_deque;
	m_nvictims++;
}

template <typename T>
void worker<T>::stop()
{
	m_stopped = true;
}

template <typename T>
T *worker<T>::root_allocate()
{
	return m_head_deque->put_allocate();
}

template <typename T>
void worker<T>::root_commit()
{
	m_head_deque->put_commit();
}

template <typename T>
void worker<T>::root_wait()
{
	local_loop(m_head_deque);
}

template <typename T>
void worker<T>::grow_tail(task_deque<T> *tail)
{
	if (tail->get_next())
		return;

	auto d = m_allocator->alloc<task_deque<T>>();
	auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
	new(d) task_deque<T>(m_taskgraph_degree, t);

	tail->set_next(d);

	// if (!m_victim_tail)
	// 	return;
    //
	// auto v = m_victim_tail->get_next();
	// if (!v) 
	// 	return;
    //
	// d->set_victim(v);
	// m_victim_tail = v;
}

template <typename T>
task_deque<T> *worker<T>::get_victim()
{
	auto i = xorshift_rand() % m_nvictims;
	return m_victims_heads[i];
}

template <typename T>
void worker<T>::steal_loop()
{
	while (m_nvictims == 0)
		std::this_thread::yield();

	auto vhead = get_victim();
	auto vtail = vhead;
	size_t now_stolen = 0;

	while (!load_relaxed(m_stopped)) {
		if (now_stolen >= m_taskgraph_degree - 1) {
			if (vtail->get_next()) {
				vtail = vtail->get_next();
				now_stolen = 0;
			}
		}

		bool was_empty = false;
		auto t = vtail->steal(&was_empty);

#if STACCATO_DEBUG
		if (t)
			COUNT(steal);
		else if (was_empty)
			COUNT(steal_empty);
		else
			COUNT(steal_race);
#endif

		if (t) {
			t->process(this, m_head_deque);
			vtail->return_stolen();

			vtail = get_victim();
			now_stolen = 0;

			continue;
		}

		if (!was_empty) {
			now_stolen++;
			continue;
		}

		if (vtail->get_next())
			vtail = vtail->get_next();
		else
			vtail = get_victim();

		now_stolen = 0;
	}
}

template <typename T>
void worker<T>::local_loop(task_deque<T> *tail)
{
	task<T> *t = nullptr;
	task_deque<T> *victim = nullptr;

	while (true) { // Local tasks loop
		if (t) {
			grow_tail(tail);

			t->process(this, tail->get_next());

			if (victim) {
				victim->return_stolen();
				victim = nullptr;
			}
		}

		size_t nstolen = 0;

		t = tail->take(&nstolen);

#if STACCATO_DEBUG
		if (t)
			COUNT(take);
		else if (nstolen == 0)
			COUNT(take_empty);
		else
			COUNT(take_stolen);
#endif

		if (t)
			continue;

		if (nstolen == 0)
			return;

		t = steal_task(tail, &victim);

		if (!t)
			std::this_thread::yield();
	}
}

template <typename T>
task<T> *worker<T>::steal_task(task_deque<T> *, task_deque<T> **victim)
{
	if (load_relaxed(m_nvictims) == 0)
		return nullptr;

	auto vhead = get_victim();
	auto vtail = vhead;
	size_t now_stolen = 0;

	while (true) {
		if (now_stolen >= m_taskgraph_degree - 1) {
			if (vtail->get_next()) {
				vtail = vtail->get_next();
				now_stolen = 0;
			}
		}

		bool was_empty = false;
		auto t = vtail->steal(&was_empty);

#if STACCATO_DEBUG
		if (t)
			COUNT(steal2);
		else if (was_empty)
			COUNT(steal2_empty);
		else
			COUNT(steal2_race);
#endif

		if (t) {
			*victim = vtail;
			return t;
		}

		if (!was_empty) {
			now_stolen++;
			continue;
		}

		if (vtail->get_next())
			vtail = vtail->get_next();
		else
			return nullptr;

		now_stolen = 0;
	}
}

#if STACCATO_DEBUG

template <typename T>
void worker<T>::print_counters()
{
	m_counter.print(m_id);
} 
#endif

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
