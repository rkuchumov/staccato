#ifndef TASK_META_HPP_34TJDRLS
#define TASK_META_HPP_34TJDRLS

#include <atomic>
#include <cstdint>
#include <functional>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <numaif.h>
#include <unistd.h>

#include "task_deque.hpp"
#include "utils.hpp"

namespace staccato
{

namespace internal {
template <typename T>
class worker;
}

template <typename T>
class task {
public:
	task();
	virtual ~task();

	virtual void execute() = 0;

	T *child();

	void spawn(T *t);

	void wait();
	
	void process(internal::worker<T> *worker, internal::task_deque<T> *tail);

	unsigned level() const;

	internal::worker<T> *worker() const;

	internal::task_deque<T> *tail() const;

	void set_worker(internal::worker<T> * w);

	void set_tail(internal::task_deque<T> *t);

	void migrate_pages(void *ptr, size_t n) const;

private:
	internal::worker<T> *m_worker;

	internal::task_deque<T> *m_tail;

	unsigned m_level;
};

template <typename T>
task<T>::task()
: m_level(0)
{ }

template <typename T>
task<T>::~task()
{ }

template <typename T>
internal::worker<T> *task<T>::worker() const
{
	return m_worker;
}

template <typename T>
internal::task_deque<T> *task<T>::tail() const
{
	return m_tail;
}

template <typename T>
void task<T>::set_worker(internal::worker<T> * w)
{
	m_worker = w;
}

template <typename T>
void task<T>::set_tail(internal::task_deque<T> *t)
{
	m_tail = t;
}

template <typename T>
void task<T>::process(internal::worker<T> *worker, internal::task_deque<T> *tail)
{
	m_worker = worker;
	m_tail = tail;
	m_tail->set_level(m_level);

	execute();
}

template <typename T>
T *task<T>::child()
{
	return m_tail->put_allocate();
}

template <typename T>
void task<T>::spawn(T *child)
{
	child->m_level = m_level + 1;
	child->m_worker = m_worker;
	child->m_tail = m_tail;

	m_tail->put_commit();
}

template <typename T>
void task<T>::wait()
{
	m_worker->local_loop(m_tail);

	// m_tail->reset();
}

template <typename T>
unsigned task<T>::level() const
{
	return m_level;
}

template <typename T>
void task<T>::migrate_pages(void *ptr, size_t n) const
{
	if (m_level != 0)
		return;

	// if (m_worker->node_id() == 0)
	// 	return;

	long page_size = sysconf(_SC_PAGESIZE);

	if (n < page_size)
		return;

	size_t nr_pages = n / page_size; 

	void **pages = new void*[nr_pages];
	int *nodes = new int[nr_pages];
	int *status = new int[nr_pages]();

	long start = ((long) ptr) & -page_size;
	for (size_t i = 0; i < nr_pages; ++i) {
		nodes[i] = 1;
		pages[i] = (void *)(start + i * page_size);
		status[i] = -EBUSY;
	}

	unsigned nr_moved = -1;
	while (nr_moved != 0 && nr_moved != nr_pages) {
		for (size_t i = 0; i < nr_pages; ++i) {
			if (status[i] != -EBUSY)
				pages[i] = nullptr;
		}

		long rc = move_pages(0, nr_pages, pages, nodes, status, 0);
		if (rc != 0)
			std::cerr << "move_pages error: " << strerror(errno) << "\n";

		nr_moved = 0;
		for (size_t i = 0; i < nr_pages; ++i) {
			if (status[i] == nodes[i])
				nr_moved++;
		}

		std::clog << "moved " << nr_moved << "\n";
	}

	delete []pages;
	delete []nodes;
	delete []status;
}


} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
