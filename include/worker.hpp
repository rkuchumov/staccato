#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

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

	void task_loop(T *waiting = nullptr, T *t = nullptr);

	bool ready() const;

	T *put_allocate();
	void put_commit();

private:
	void init(
		size_t id,
		size_t nworkers,
		worker<T> **workers
	);

	void wait_collegues_init();

    void inc_tail();

    void dec_tail();

    size_t predict_page_size();

	const size_t m_taskgraph_degree;
	const size_t m_taskgraph_height;

	std::thread *m_thread;
	std::atomic_bool m_ready;
	lifo_allocator *m_allocator;

	struct collegue_t {
		worker<T> *w;
		task_deque<T> *head;
		size_t level;
	};

	size_t m_ncollegues;
	collegue_t *m_collegues;

	task_deque<T> *m_head_deque;

	// TODO: dont use this var, store in stack instead
	task_deque<T> *m_tail_deque;
};

template <typename T>
worker<T>::worker(size_t taskgraph_degree, size_t taskgraph_height)
: m_taskgraph_degree(taskgraph_degree)
, m_taskgraph_height(taskgraph_height)
, m_thread(nullptr)
, m_ready(false)
, m_allocator(nullptr)
, m_head_deque(nullptr)
, m_tail_deque(nullptr)
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
			// Debug() << "Starting worker::tasks_loop()";
			task_loop();
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

	// TODO: allocate all tree structure
	auto d = m_allocator->alloc<task_deque<T>>();
	auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
	new(d) task_deque<T>(t);

	m_head_deque = d;
	m_tail_deque = d;
}

template <typename T>
inline size_t worker<T>::predict_page_size()
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
	for (size_t i = 0; i < m_ncollegues; ++i)
		while (!m_collegues[i].w->m_ready)
			std::this_thread::yield();
}

template <typename T>
void worker<T>::join()
{
	m_thread->join();
}

template <typename T>
void worker<T>::task_loop(T *waiting, T *t)
{
	while (true) { // Local tasks loop
		if (t) { 
			inc_tail();

			t->process(this);

			dec_tail();
		}

		if (waiting && waiting->finished())
			return;

		t = m_tail_deque->take();

		if (!t)
			break;
	}
}

template <typename T>
void worker<T>::inc_tail()
{
	if (!m_tail_deque->get_next()) {
		auto d = m_allocator->alloc<task_deque<T>>();
		auto t = m_allocator->alloc_array<T>(m_taskgraph_degree);
		new(d) task_deque<T>(t);

		d->set_prev(m_tail_deque);
		m_tail_deque->set_next(d);
	}

	m_tail_deque = m_tail_deque->get_next();
	m_tail_deque->set_null(false);
}

template <typename T>
void worker<T>::dec_tail()
{
	if (!m_tail_deque->get_prev())
		return; // TODO: assert(false)???

	m_tail_deque->set_null(true);
	m_tail_deque = m_tail_deque->get_prev();
}

template <typename T>
T *worker<T>::put_allocate()
{
	return m_tail_deque->put_allocate();
}

template <typename T>
void worker<T>::put_commit()
{
	m_tail_deque->put_commit();
}

}
}

#endif /* end of include guard: WORKER_H_MIRBQTTK */
