#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>
#include <limits>

#include "task_deque.hpp"
#include "lifo_allocator.hpp"
#include "constants.hpp"
#include "task.hpp"

namespace staccato
{
namespace internal
{

template <typename T>
class worker
{
public:
	worker(size_t taskgraph_degree, size_t taskgraph_height);
	~worker();

	void async_init(
		size_t id,
		size_t nworkers,
		worker<T> **workers
	);

	void join();

	bool ready() const;

	void local_loop(task_deque<T> *tail);

	void steal_loop(task_deque<T> *tail);

	T *root_allocate();
	void root_commit();
	void root_wait();

private:
    inline size_t predict_page_size() const;

	void init(
		size_t id,
		size_t nworkers,
		worker<T> **workers
	);

	void wait_collegues_init();

    void grow_tail(task_deque<T> *tail);

    task<T> *steal_rnd_task(task_deque<T> **victim);

	struct collegue_t {
		worker<T> *w;
		task_deque<T> *head;
		size_t level;
	};

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	std::thread *m_thread;

	std::atomic_bool m_joined;

	std::atomic_bool m_ready;

	size_t m_ncollegues;
	collegue_t *m_collegues;

	lifo_allocator *m_allocator;

	task_deque<T> *m_head_deque;
};

template <typename T>
worker<T>::worker(size_t taskgraph_degree, size_t taskgraph_height)
: m_taskgraph_degree(taskgraph_degree)
, m_taskgraph_height(taskgraph_height)
, m_thread(nullptr)
, m_joined(false)
, m_ready(false)
, m_allocator(nullptr)
, m_head_deque(nullptr)
{ }

template <typename T>
worker<T>::~worker()
{
	delete m_thread;
	delete m_allocator;
}

template <typename T>
void worker<T>::async_init(
	size_t id,
	size_t nworkers,
	worker<T> **workers
) {
	// TODO: use sync barrier

	if (id == 0) {
		init(id, nworkers, workers);
		m_ready = true;
		wait_collegues_init();
		return;
	}

	m_thread = new std::thread([=] {
			init(id, nworkers, workers);
			m_ready = true;
			wait_collegues_init();
			steal_loop(m_head_deque);
		}
	);
}

template <typename T>
void worker<T>::init(
	size_t id,
	size_t nworkers,
	worker<T> **workers
) {
	m_ncollegues = nworkers - 1;

	m_allocator = new lifo_allocator(predict_page_size());

	m_collegues = m_allocator->alloc_array<collegue_t>(m_ncollegues);
	new(m_collegues) collegue_t[m_ncollegues];

	for (size_t i = 0, j = 0; i < nworkers; ++i) {
		if (i == id)
			continue;

		m_collegues[j].w = workers[i];
		++j;
	}

	auto d = m_allocator->alloc<task_deque<T>>();
	auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
	new(d) task_deque<T>(t);
	d->set_null(false);

	m_head_deque = d;

	for (size_t i = 1; i < m_taskgraph_height; ++i) {
		grow_tail(d);
		d = d->get_next();
	}
}

template <typename T>
inline size_t worker<T>::predict_page_size() const
{
	size_t c = 0;
	c += alignof(collegue_t) + sizeof(collegue_t) * m_ncollegues;

	size_t d = 0;
	d += alignof(task_deque<T>) + sizeof(task_deque<T>);
	d += alignof(T) + sizeof(T) * m_taskgraph_degree;
	d *= m_taskgraph_height;

	return c + d;
}

template <typename T>
void worker<T>::wait_collegues_init()
{
	for (size_t i = 0; i < m_ncollegues; ++i) {
		while (!m_collegues[i].w->m_ready)
			std::this_thread::yield();

		// TODO: move it to other function?
		m_collegues[i].head = m_collegues[i].w->m_head_deque;
		m_collegues[i].level = std::numeric_limits<std::size_t>::max();
	}
}

template <typename T>
void worker<T>::join()
{
	m_joined = true;
	m_thread->join();
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
	new(d) task_deque<T>(t);

	d->set_prev(tail);
	tail->set_next(d);
}

template <typename T>
task<T> *worker<T>::steal_rnd_task(task_deque<T> **victim)
{
	auto i = xorshift_rand() % m_ncollegues;
	auto h = m_collegues[i].w->m_head_deque;

	bool was_empty;
	bool was_null;

	while (h) {
		was_empty = true;
		was_null = true;

		auto t = h->steal(&was_empty, &was_empty);

		if (t) {
			*victim = h;
			return t;
		}

		if (was_null)
		{
			h = h->get_prev();
			return nullptr;
		}

		if (was_empty)
			h = h->get_next();

		return nullptr;
	}

	Debug() << "aa``";
	return nullptr;
}

// template <typename T>
// task_deque<T> *worker<T>::min_level_victim() const
// {
// 	size_t min_level = std::numeric_limits<std::size_t>::max();
// 	task_deque<T> *t = nullptr;
// 	for (size_t i = 0; i < m_ncollegues; ++i) {
// 		if (m_collegues[i].level > min_level)
// 			continue;
//
// 		min_level = m_collegues[i].level;
// 		t = m_collegues[i].head;
// 	}
//
// 	return t;
// }
//

template <typename T>
void worker<T>::steal_loop(task_deque<T> *tail)
{
	while (!load_relaxed(m_joined)) { // Local tasks loop
		task_deque<T> *victim = nullptr;
		auto t = steal_rnd_task(&victim);

		if (!t)
			continue;

		// XXX: porbably instuction cache-miss is caused by 
		// the following call
		grow_tail(tail);

		t->process(this, tail->get_next(), tail->get_level());

		victim->return_stolen();
	}
}

template <typename T>
void worker<T>::local_loop(task_deque<T> *tail)
{
	task<T> *t = nullptr;
	task_deque<T> *victim = nullptr;

	while (true) { // Local tasks loop
		if (t) {
			// XXX: porbably instuction cache-miss is caused by 
			// the following call
			grow_tail(tail);

			t->process(this, tail->get_next(), tail->get_level());

			if (victim) {
				victim->return_stolen();
				victim = nullptr;
			}
		}

		t = tail->take();

		if (t)
			continue;

		if (!tail->have_stolen())
			return;

		t = steal_rnd_task(&victim);
	}
}

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
