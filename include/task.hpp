#ifndef TASK_META_HPP_34TJDRLS
#define TASK_META_HPP_34TJDRLS

#include <atomic>
#include <cstdint>
#include <functional>
#include <cstdlib>

#include "constants.hpp"
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

	// size_t level() const;

private:
	internal::worker<T> *m_worker;

	internal::task_deque<T> *m_tail;

	// size_t m_level;
};

template <typename T>
task<T>::task()
: m_worker(nullptr)
// , m_level(0)
{ }

template <typename T>
task<T>::~task()
{ }

template <typename T>
void task<T>::process(internal::worker<T> *worker, internal::task_deque<T> *tail)
{
	m_worker = worker;
	m_tail = tail;

	m_tail->set_null(false);

	execute();

	m_tail->set_null(true);
}

// template <typename T>
// size_t task<T>::level() const
// {
// 	return m_level;
// }

template <typename T>
T *task<T>::child()
{
	return m_tail->put_allocate();
}

template <typename T>
void task<T>::spawn(T *)
{
	// t->m_level = m_level + 1;

	m_tail->put_commit();
}

template <typename T>
void task<T>::wait()
{
	m_worker->task_loop(nullptr, m_tail);
}

} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
