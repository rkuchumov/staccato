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

template <typename T, size_t N = 4>
class task_mailbox
{
public:
	task_mailbox();
	~task_mailbox();

	size_t size() const;

	void put(T *task);

	T *take();

private:
	T *m_array[N];

	std::atomic_size_t m_top;
	std::atomic_size_t m_bottom;
	// STACCATO_ALIGN std::atomic_size_t m_bottom;
};

template <typename T, size_t N>
task_mailbox<T, N>::task_mailbox()
: m_top(1)
, m_bottom(1)
{
	STACCATO_ASSERT(is_pow2(N), "Deque size is not power of 2");
}

template <typename T, size_t N>
task_mailbox<T, N>::~task_mailbox()
{ }

template <typename T, size_t N>
void task_mailbox<T, N>::put(T *task)
{
	auto b = load_relaxed(m_bottom);
	m_array[b & (N - 1)] = task;

	atomic_fence_release();
	store_relaxed(m_bottom, b + 1);
}

template <typename T, size_t N>
T *task_mailbox<T, N>::take()
{
	auto t = load_relaxed(m_top);
	auto b = load_relaxed(m_bottom);

	if (t >= b)
		return nullptr;

	T *task = m_array[t & (N - 1)];

	atomic_fence_release();
	store_relaxed(m_top, t + 1);
	return task;
}

template <typename T, size_t N>
size_t task_mailbox<T, N>::size() const
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
