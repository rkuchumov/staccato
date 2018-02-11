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

	void async_init(worker<T> *victim = nullptr);

	void join();

	bool ready() const;

	void local_loop(task_deque<T> *tail);

	void steal_loop();

	T *root_allocate();
	void root_commit();
	void root_wait();

private:
    inline size_t predict_page_size() const;

	void init(worker<T> *victim);

	void sync_with_victim();

    void grow_tail(task_deque<T> *tail);

	// TODO: pass this as template parameteres 
	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	lifo_allocator *m_allocator;

	std::thread *m_thread;

	std::atomic_bool m_joined;

	std::atomic_bool m_ready;

	worker<T> *m_victim;

	task_deque<T> *m_victim_head;

	task_deque<T> *m_head_deque;
};

template <typename T>
worker<T>::worker(size_t taskgraph_degree, size_t taskgraph_height)
: m_taskgraph_degree(taskgraph_degree)
, m_taskgraph_height(taskgraph_height)
, m_allocator(nullptr)
, m_thread(nullptr)
, m_joined(false)
, m_ready(false)
, m_head_deque(nullptr)
{ }

template <typename T>
worker<T>::~worker()
{
	delete m_thread;
	delete m_allocator;
}

template <typename T>
void worker<T>::async_init(worker<T> *victim)
{
	// TODO: use sync barrier

	if (!victim) {
		init(nullptr);
		m_ready = true;
		sync_with_victim();
		return;
	}

	m_thread = new std::thread([=] {
			init(victim);
			m_ready = true;
			sync_with_victim();
			steal_loop();
		}
	);
}

template <typename T>
void worker<T>::init(worker<T> *victim)
{
	m_victim = victim;

	m_allocator = new lifo_allocator(predict_page_size());

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
	size_t s = 0;
	s += alignof(task_deque<T>) + sizeof(task_deque<T>);
	s += alignof(T) + sizeof(T) * m_taskgraph_degree;
	s *= m_taskgraph_height;
	return s;
}

template <typename T>
void worker<T>::sync_with_victim()
{
	if (!m_victim)
		return;

	// TODO: there's a right way of doing this 
	while (!m_victim->m_ready)
		std::this_thread::yield();

	auto h = m_head_deque;
	m_victim_head = m_victim->m_head_deque;
	auto p = m_victim_head;

	while (h && p) {
		h->set_victim(p);

		p = p->get_next();
		h = h->get_next();
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
void worker<T>::steal_loop()
{
	auto vtail = m_victim_head;

	while (!load_relaxed(m_joined)) { // Local tasks loop
		bool was_null = false;
		bool was_empty = false;

		auto t = vtail->steal(&was_empty, &was_null);

		if (t) {
			t->process(this, m_head_deque);
			vtail->return_stolen();
			continue;
		}

		if (was_empty) {
			if (vtail->get_next())
				vtail = vtail->get_next();
		}

		if (was_null) {
			if (vtail->get_prev())
				vtail = vtail->get_prev();
		}
	}
}

template <typename T>
void worker<T>::local_loop(task_deque<T> *tail)
{
	task<T> *t = nullptr;
	task_deque<T> *victim = nullptr;

	while (true) { // Local tasks loop
		if (t) {
			if (!tail->get_next())
			// XXX: porbably instuction cache-miss is caused by 
			// the following call
			grow_tail(tail);

			t->process(this, tail->get_next());

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

		// t = steal_rnd_task(&victim);
	}
}

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
