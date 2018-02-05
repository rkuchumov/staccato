#ifndef WORKER_H_MIRBQTTK
#define WORKER_H_MIRBQTTK

#include <cstdlib>
#include <thread>

#include "task_deque.hpp"
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
	worker();
	~worker();

	void async_init(
		size_t id,
		size_t nworkers,
		worker<T> **workers,
		size_t max_degree
	);

	void join();

	void task_loop(T *waiting = nullptr, T *t = nullptr);

	bool ready() const;

    // TODO: rename
    void inc_tail();
    void dec_tail();

	T *put_allocate();
	void put_commit();

private:
	void init(
		size_t id,
		size_t nworkers,
		worker<T> **workers,
		size_t max_degree
	);

	void wait_collegues_init();

	std::thread *m_thread;
	std::atomic_bool m_ready;

	struct collegue_t {
		worker<T> *w;
		task_deque<T> *head;
		size_t level;
	};

	size_t m_ncollegues;
	collegue_t *m_collegues;

	task_deque<T> *m_head_deque;
	task_deque<T> *m_tail_deque;
};

template <typename T>
worker<T>::worker()
: m_thread(nullptr)
, m_head_deque(nullptr)
, m_tail_deque(nullptr)
{
	delete m_thread;

	// TODO: delete allocator instead
	delete []m_collegues;

	auto d = m_head_deque;
	while (d) {
		auto t = d;
		d = d->get_next();
		delete t;
	}
}

template <typename T>
worker<T>::~worker()
{ }

template <typename T>
void worker<T>::async_init(
	size_t id,
	size_t nworkers,
	worker<T> **workers,
	size_t max_degree
) {
	// TODO: use sync barrier

	if (id == 0) {
		init(id, nworkers, workers, max_degree);
		m_ready = true;
		wait_collegues_init();
		return;
	}

	m_thread = new std::thread([=] {
			init(id, nworkers, workers, max_degree);
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
	worker<T> **workers,
	size_t max_degree
)
{
	// TODO: allocate allocator
	auto d = new task_deque<T>(max_degree);
	m_head_deque = d;
	m_tail_deque = d;
	// TODO: allocate all tree structure

	m_ncollegues = nworkers - 1;
	m_collegues = new collegue_t[m_ncollegues];

	for (size_t i = 0, j = 0; i < nworkers; ++i) {
		if (i == id)
			continue;

		m_collegues[j].w = workers[i];
		++j;
	}
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
		if (t)
			t->process(this);

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
	if (!m_tail_deque->get_next())
		m_tail_deque->create_next(); // TODO: dont' create here

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
