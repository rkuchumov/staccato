#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>
#include <limits>
#include <limits>

#include <pthread.h>

#include "utils.hpp"

#include "task_deque.hpp"
#include "lifo_allocator.hpp"
#include "task.hpp"
#include "task_mailbox.hpp"
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
		int node_id,
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

	void spawn_root(T *root);

	uint64_t get_deque_load() const;

	void count_task_deque(unsigned level);
	void uncount_task_deque(unsigned level);

	void mail_task(T *task);

	const T *get_mailed_task() const;

	void print_load(uint64_t);

#if STACCATO_DEBUG
	size_t min_power(uint64_t load);
	static unsigned get_tasks_at_level(uint64_t load, unsigned level);

	const counter &get_counter() const;
#endif

	task_deque<T> *get_deque();

	inline int node_id() const {
		return m_node_id;
	}

	static void compute_powers(uint64_t taskgraph_degree);

// private:
	void init(size_t core_id, worker<T> *victim);

	void grow_tail(task_deque<T> *tail);

	task_deque<T> *get_victim_head();

	task<T> *steal_task();

	static uint64_t m_n[64];
	static unsigned m_d_max;

	const size_t m_id;

	const int m_node_id;

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;
	lifo_allocator *m_allocator;

#if STACCATO_DEBUG
	counter m_counter;
#endif

	std::atomic_bool m_stopped;

	std::atomic_bool m_assigned;

	std::atomic_size_t m_nvictims;

	task_deque<T> **m_victim_heads;

	std::atomic<T *> m_mailed_task;

	task_deque<T> *m_head;

	std::atomic_ullong m_deque_load;
};

template <typename T>
void worker<T>::print_load(uint64_t load)
{
	for (size_t i = worker<T>::m_d_max; i != 0; i--) {
		unsigned n = 0;
		while (load >= worker<T>::m_n[i]) {
			load -= worker<T>::m_n[i];
			n++;
		}

		std::cerr << n << "x" << worker<T>::m_d_max-i << " + ";
	}
}

template <typename T>
uint64_t worker<T>::m_n[64];

template <typename T>
unsigned worker<T>::m_d_max;

template <typename T>
void worker<T>::compute_powers(uint64_t taskgraph_degree)
{
	m_n[0] = 1;
	m_d_max = 62;

	uint64_t s = 1;
	uint64_t d = taskgraph_degree;

	for (int i = 1; i < 64; ++i) {
		s += d;
		d *= taskgraph_degree;

		worker<T>::m_n[i] = s;

		if (m_n[i] > m_n[i-1])
			continue;

		m_d_max = i - 2;
		break;
	}

	// for (int i = 0; i < m_d_max+2; ++i)
	// 	std::clog << "n(" <<  i <<  ") = " << m_n[i] << "\n";
    //
	// std::cerr << "max id = " << m_d_max << "\n";

}

template <typename T>
worker<T>::worker(
	size_t id,
	int core_id,
	int node_id,
	lifo_allocator *alloc,
	size_t nvictims,
	size_t taskgraph_degree,
	size_t taskgraph_height
)
: m_id(id)
, m_node_id(node_id)
, m_taskgraph_degree(taskgraph_degree)
, m_taskgraph_height(taskgraph_height)
, m_allocator(alloc)
, m_stopped(false)
, m_assigned(false)
, m_nvictims(0)
, m_victim_heads(nullptr)
, m_mailed_task(nullptr)
, m_head(nullptr)
, m_deque_load(0)
{
	m_victim_heads = m_allocator->alloc_array<task_deque<T> *>(nvictims);

	auto d = m_allocator->alloc<task_deque<T>>();
	auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
	new(d) task_deque<T>(m_taskgraph_degree, t);

	m_head = d;

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
	m_victim_heads[m_nvictims] = victim->m_head;
	m_nvictims++;
}

template <typename T>
void worker<T>::stop()
{
	m_stopped = true;
}

template <typename T>
void worker<T>::spawn_root(T *root)
{
	root->process(this, m_head);
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
internal::task_deque<T> *worker<T>::get_victim_head()
{
	auto i = xorshift_rand() % m_nvictims;
	return m_victim_heads[i];
}

template <typename T>
void worker<T>::steal_loop()
{
	while (m_nvictims == 0)
		std::this_thread::yield();

	task<T> *t = nullptr;
	task_deque<T> *victim = nullptr;
	size_t now_stolen = 0;

	while (!load_relaxed(m_stopped)) {
		if (t) {
			t->process(this, m_head);

			victim->return_stolen();

			victim = nullptr;
			now_stolen = 0;
		}

		t = load_acquire(m_mailed_task);

		if (t) {
			store_release(m_mailed_task, nullptr);
			victim = t->tail();
			store_relaxed(m_assigned, true);
			continue;
		}

		if (load_relaxed(m_assigned) == false)
			continue;

		if (victim == nullptr)
			victim = get_victim_head();

		if (now_stolen >= m_taskgraph_degree - 1) {
			if (victim->get_next()) {
				victim = victim->get_next();
				now_stolen = 0;
			}
		}

		bool was_empty = false;
		t = victim->steal(&was_empty);

#if STACCATO_DEBUG
		if (t)
			COUNT(steal);
		else if (was_empty)
			COUNT(steal_empty);
		else
			COUNT(steal_race);
#endif

		if (t) {
			t->worker()->uncount_task_deque(t->level());
			continue;
		}

		if (!was_empty) {
			now_stolen++;
			continue;
		}

		if (victim->get_next())
			victim = victim->get_next();
		else
			victim = get_victim_head();

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

			if (victim)
				victim->return_stolen();
		}

		size_t nstolen = 0;

		t = tail->take(&nstolen);

#if STACCATO_DEBUG
		if (t) {
			COUNT(take);
		} else if (nstolen == 0) {
			COUNT(take_empty);
		} else {
			COUNT(take_stolen);
		}
#endif

		if (t) {
#if STACCATO_DEBUG
			if (t->level() < worker<T>::m_d_max && get_tasks_at_level(m_deque_load, t->level()) == 0) {
				std::cerr << t->level() << " ";
				print_load(m_deque_load);
				std::cerr << "\n";
				STACCATO_ASSERT(get_tasks_at_level(m_deque_load, t->level()) > 0, "inconsistent m_deque_load");
			}
#endif

			uncount_task_deque(t->level());

			victim = nullptr;
			continue;
		}

		if (nstolen == 0)
			return;

		t = load_acquire(m_mailed_task);
		if (t) {
			store_release(m_mailed_task, nullptr);
			victim = t->tail();
			continue;
		}

		t = steal_task();

		if (t) {
			t->worker()->uncount_task_deque(t->level());
			victim = t->tail();
			continue;
		}

		std::this_thread::yield();
	}
}

template <typename T>
task<T> *worker<T>::steal_task()
{
	if (load_relaxed(m_nvictims) == 0)
		return nullptr;

	auto vt = get_victim_head();
	size_t now_stolen = 0;

	while (true) {
		if (now_stolen >= m_taskgraph_degree - 1) {
			if (vt->get_next()) {
				vt = vt->get_next();
				now_stolen = 0;
			}
		}

		bool was_empty = false;
		auto t = vt->steal(&was_empty);

#if STACCATO_DEBUG
		if (t)
			COUNT(steal2);
		else if (was_empty)
			COUNT(steal2_empty);
		else
			COUNT(steal2_race);
#endif

		if (t)
			return t;

		if (!was_empty) {
			now_stolen++;
			continue;
		}

		if (!vt->get_next())
			return nullptr;

		vt = vt->get_next();
		now_stolen = 0;
	}
}

template <typename T>
unsigned worker<T>::get_tasks_at_level(uint64_t load, unsigned level)
{
	for (size_t i = worker<T>::m_d_max; i != 0; i--) {
		if (!load)
			break;

		unsigned n = 0;
		while (load >= worker<T>::m_n[i]) {
			load -= worker<T>::m_n[i];
			n++;
		}

		if (worker<T>::m_d_max-i == level)
			return n;
	}

	return 0;
}

template <typename T>
void worker<T>::count_task_deque(unsigned level)
{
	if (level >= m_d_max) {
		return;
	}

	level = m_d_max - level;

	m_deque_load += m_n[level];
}

template <typename T>
void worker<T>::uncount_task_deque(unsigned level)
{
	if (level >= m_d_max) {
		return;
	}

	level = m_d_max - level;

	STACCATO_ASSERT(m_n[level] <= m_deque_load, "inconsistent m_deque_load state");
	m_deque_load -= m_n[level];
}

template <typename T>
uint64_t worker<T>::get_deque_load() const
{
	return load_acquire(m_deque_load);
}

template <typename T>
task_deque<T> *worker<T>::get_deque()
{
	return m_head;
}

template <typename T>
const T *worker<T>::get_mailed_task() const
{
	return load_relaxed(m_mailed_task);
}

template <typename T>
void worker<T>::mail_task(T *task)
{
	STACCATO_ASSERT(!m_mailed_task, "a task is already mailed");
	store_release(m_mailed_task, task);
}

#if STACCATO_DEBUG

template <typename T>
const counter &worker<T>::get_counter() const
{
	return m_counter;
} 

template <typename T>
size_t worker<T>::min_power(uint64_t load)
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

#endif

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
