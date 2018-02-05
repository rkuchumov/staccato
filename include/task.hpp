#ifndef TASK_META_HPP_34TJDRLS
#define TASK_META_HPP_34TJDRLS

#include <atomic>
#include <cstdint>
#include <functional>
#include <cstdlib>

#include "constants.hpp"
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
	
	void process(internal::worker<T> *worker);

	bool finished();

private:
	internal::worker<T> *m_worker;
	T *m_parent;

	std::atomic_size_t m_nsubtasks;
};

template <typename T>
task<T>::task()
: m_worker(nullptr)
, m_parent(nullptr)
, m_nsubtasks(0)
{ }

template <typename T>
task<T>::~task()
{ }

template <typename T>
void task<T>::process(internal::worker<T> *worker)
{
	m_worker = worker;

	m_worker->inc_tail();

	execute();

	m_worker->dec_tail();

	if (m_parent != nullptr)
		dec_relaxed(m_parent->m_nsubtasks);
}

template <typename T>
bool task<T>::finished()
{
	return load_relaxed(m_nsubtasks) == 0;
}

template <typename T>
T *task<T>::child()
{
	return m_worker->put_allocate();
}

template <typename T>
void task<T>::spawn(T *t)
{
	inc_relaxed(m_nsubtasks);

	t->m_parent = reinterpret_cast<T *>(this);

	m_worker->put_commit();
}

template <typename T>
void task<T>::wait()
{
	m_worker->task_loop(reinterpret_cast<T *>(this));
}

} /* staccato */ 


#endif /* end of include guard: TASK_META_HPP_34TJDRLS */
