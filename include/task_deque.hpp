#ifndef STACCATO_DEQUE_H
#define STACCATO_DEQUE_H

#include <atomic>
#include <cstddef>
#include <cstring>

#include "constants.hpp"
#include "utils.hpp"

namespace staccato
{

namespace internal
{

template <typename T>
class task_deque
{
public:
	task_deque(size_t size);
	~task_deque();

	void create_next();
	task_deque<T> *get_next();
	task_deque<T> *get_prev();

	bool is_null();
	void set_null(bool null);

	T *put_allocate();
	void put_commit();

	T *take();
	T *steal(bool *was_empty);

private:
	const size_t m_size;
	T * m_array;

	task_deque<T> *m_next;
	task_deque<T> *m_prev;

	STACCATO_ALIGN std::atomic<T *> m_top;
	STACCATO_ALIGN std::atomic<T *> m_bottom;
};

template <typename T>
task_deque<T>::task_deque(size_t size)
: m_size(size)
, m_array(nullptr)
, m_next(nullptr)
, m_prev(nullptr)
{
	m_array = reinterpret_cast<T *> (new char[size * sizeof(T)]);
	store_relaxed(m_top, m_array + 1);
	store_relaxed(m_bottom, m_array + 1);
}

template <typename T>
task_deque<T>::~task_deque()
{
	// delete []m_array;
}

template <typename T>
void task_deque<T>::create_next()
{
	auto n = new task_deque<T>(m_size);
	n->m_prev = this;
	m_next = n;
}

template <typename T>
task_deque<T> *task_deque<T>::get_next()
{
	return m_next;
}

template <typename T>
task_deque<T> *task_deque<T>::get_prev()
{
	return m_prev;
}

template <typename T>
T *task_deque<T>::put_allocate()
{
	auto b = load_relaxed(m_bottom);
	return b - 1;
}

template <typename T>
void task_deque<T>::put_commit()
{
	auto b = load_relaxed(m_bottom);
	atomic_fence_release();
	store_relaxed(m_bottom, b + 1);
}

template <typename T>
T *task_deque<T>::take()
{
	auto b = dec_relaxed(m_bottom) - 1;
	auto t = load_relaxed(m_top);

	// Check whether the deque was empty
	if (t > b) {
		// Restoring to empty state
		store_relaxed(m_bottom, b + 1);
		return nullptr;
	}

	// Check if the task can be stolen
	if (t == b) {
		// Check if it's not stolen
		if (!cas_strong(m_top, t, t + 1)) {
			// It was stolen, restoring to previous state
			m_bottom = b + 1;
			return nullptr;
		}

		// Wasn't stolen, but we icnremented top index
		m_bottom = b + 1;
		return b - 1;
	}

	// The task can't be stolen, no need for CAS
	return b - 1;
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

	// Check if loaded task is not stolen
	if (!cas_weak(m_top, t, t + 1))
		return nullptr;

	return t - 1;
}

template <typename T>
void task_deque<T>::set_null(bool null)
{
	if (null) {
		store_relaxed(m_top, nullptr);
	} else {
		store_relaxed(m_top, m_array + 1);
		store_relaxed(m_bottom, m_array + 1);
	}
}

template <typename T>
bool task_deque<T>::is_null()
{
	return load_relaxed(m_top) == nullptr;
}

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
