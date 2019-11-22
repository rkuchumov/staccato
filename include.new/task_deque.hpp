#ifndef TASK_DEQUE_HPP_1ZDWEADG
#define TASK_DEQUE_HPP_1ZDWEADG

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
class task_deque
{
public:
	task_deque(size_t size, T *mem);
	~task_deque();

	void set_next(task_deque<T> *d);
	void set_victim(task_deque<T> *d);

	task_deque<T> *get_next();

	void return_stolen();

	T *put_allocate();
	void put_commit();

	T *take(size_t *);
	T *steal(bool *was_empty);

	size_t level() const;
	void set_level(size_t level);

private:
	const size_t m_mask;

	// TODO: make this array a part of this class
	T * m_array;

	task_deque<T> *m_next;

	STACCATO_ALIGN std::atomic_size_t m_level;
	STACCATO_ALIGN std::atomic_size_t m_nstolen;
	STACCATO_ALIGN std::atomic_size_t m_top;
	STACCATO_ALIGN std::atomic_size_t m_bottom;
};

template <typename T>
task_deque<T>::task_deque(size_t size, T *mem)
: m_mask(size - 1)
, m_array(mem)
, m_next(nullptr)
, m_nstolen(0)
, m_top(1)
, m_bottom(1)
{
	STACCATO_ASSERT(is_pow2(size), "Deque size is not power of 2");
}

template <typename T>
task_deque<T>::~task_deque()
{ }

template <typename T>
void task_deque<T>::set_next(task_deque<T> *d)
{
	m_next = d;
}

template <typename T>
task_deque<T> *task_deque<T>::get_next()
{
	return m_next;
}

template <typename T>
T *task_deque<T>::put_allocate()
{
	auto b = load_relaxed(m_bottom);
	return &m_array[b & m_mask];
}

template <typename T>
void task_deque<T>::put_commit()
{
	auto b = load_relaxed(m_bottom);
	atomic_fence_release();
	store_relaxed(m_bottom, b + 1);
}

template <typename T>
T *task_deque<T>::take(size_t *nstolen)
{
	auto b = dec_relaxed(m_bottom) - 1;
	auto t = load_relaxed(m_top);
	auto n = load_relaxed(m_nstolen);

	// Check whether the deque was empty
	if (t > b) {
		// Restoring to empty state
		store_relaxed(m_bottom, b + 1);
		*nstolen = n;
		return nullptr;
	}

	// Check if the task can be stolen
	if (t == b) {
		// Check if it's not stolen
		if (!cas_strong(m_top, t, t + 1)) {
			// It was stolen, restoring to previous state
			m_bottom = b + 1;
			*nstolen = n + 1;
			return nullptr;
		}

		// Wasn't stolen, but we icnremented top index
		m_bottom = b + 1;
		return &m_array[b & m_mask];
	}

	// The task can't be stolen, no need for CAS
	return &m_array[b & m_mask];
}
 
template <typename T>
T *task_deque<T>::steal(bool *was_empty)
{
	auto t = load_acquire(m_top);
	atomic_fence_seq_cst();
	auto b = load_acquire(m_bottom);

	// Check if deque was empty
	if (t >= b) {
		*was_empty = true;
		return nullptr;
	} 

	auto r = &m_array[t & m_mask];

	inc_relaxed(m_nstolen);

	// Check if loaded task is not stolen
	if (!cas_weak(m_top, t, t + 1)) {
		dec_relaxed(m_nstolen);
		return nullptr;
	}

	return r;
}

template <typename T>
void task_deque<T>::set_level(size_t level)
{
	// store_release(m_level, level);
	m_level = level;
}

template <typename T>
size_t task_deque<T>::level() const
{
	// return load_acquire(m_level);
	return m_level;
}

template <typename T>
void task_deque<T>::return_stolen()
{
	STACCATO_ASSERT(m_nstolen > 0, "Decrementing stolen count when there are no stolen tasks");
	dec_relaxed(m_nstolen);
}

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: TASK_DEQUE_HPP_1ZDWEADG */
