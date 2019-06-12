#ifndef TASK_MAILBOX_HPP_RNPZEXES
#define TASK_MAILBOX_HPP_RNPZEXES


#include <atomic>
#include <cstddef>
#include <cstring>

#include "utils.hpp"
#include "debug.hpp"
#include "lifo_allocator.hpp"

namespace staccato
{

namespace internal
{

template <typename T>
class task_mailbox
{
public:
	task_mailbox(size_t size, T **mem);
	~task_mailbox();

	void return_stolen();

	T *put_allocate();
	void put_commit();

	void put(T *task);

	size_t size() const;

	T *take();
	T *steal(bool *was_empty);

private:
	const size_t m_mask;

	T **m_array;

	std::atomic_size_t m_top;
	std::atomic_size_t m_bottom;
	// STACCATO_ALIGN std::atomic_size_t m_bottom;
};

template <typename T>
task_mailbox<T>::task_mailbox(size_t size, T **mem)
: m_mask(size - 1)
, m_array(mem)
, m_top(1)
, m_bottom(1)
{
	STACCATO_ASSERT(is_pow2(size), "Deque size is not power of 2");
}

template <typename T>
task_mailbox<T>::~task_mailbox()
{ }

template <typename T>
void task_mailbox<T>::put(T *task)
{
	auto b = load_relaxed(m_bottom);
	m_array[b & m_mask] = task;

	atomic_fence_release();
	store_relaxed(m_bottom, b + 1);
}

template <typename T>
T *task_mailbox<T>::take()
{
	auto t = load_relaxed(m_top);
	auto b = load_relaxed(m_bottom);

	if (t >= b)
		return nullptr;

	T *task = m_array[t & m_mask];

	atomic_fence_release();
	store_relaxed(m_top, t + 1);
	return task;
}

template <typename T>
size_t task_mailbox<T>::size() const
{
	auto t = load_relaxed(m_top);
	auto b = load_relaxed(m_bottom);

	if (t >= b)
		return 0;

	return b - t;
}
 
} // namespace internal
} // namespace stacccato

#endif /* end of include guard: TASK_MAILBOX_HPP_RNPZEXES */
