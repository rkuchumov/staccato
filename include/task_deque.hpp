#ifndef STACCATO_DEQUE_H
#define STACCATO_DEQUE_H

#include <atomic>
#include <cstddef>
#include <cstring>

#include "constants.hpp"
#include "utils.hpp"
#include "lifo_allocator.hpp"

namespace staccato
{

namespace internal
{

// TODO: remame, it's not only a deque
template <typename T>
class task_deque
{
public:
	task_deque(T *mem);
	~task_deque();

	void set_prev(task_deque<T> *d);
	void set_next(task_deque<T> *d);
	void set_victim(task_deque<T> *d);

	task_deque<T> *get_prev();
	task_deque<T> *get_next();
	task_deque<T> *get_victim();

	bool have_stolen() const;
	void return_stolen();

	bool null() const;
	void set_null(bool null);

	void reset();

	T *put_allocate();
	void put_commit();

	T *take(bool *was_stolen);
	T *steal(bool *was_empty, bool *was_null);

	void set_level(size_t level);

private:
	T * m_array;

	task_deque<T> *m_prev;
	task_deque<T> *m_next;
	task_deque<T> *m_victim;

	STACCATO_ALIGN std::atomic_size_t m_nstolen;
	STACCATO_ALIGN std::atomic<T *> m_top;
	STACCATO_ALIGN std::atomic<T *> m_bottom;
};

template <typename T>
task_deque<T>::task_deque(T *mem)
: m_array(mem)
, m_prev(nullptr)
, m_next(nullptr)
, m_victim(nullptr)
, m_nstolen(0)
{ }

template <typename T>
task_deque<T>::~task_deque()
{ }

template <typename T>
void task_deque<T>::set_prev(task_deque<T> *d)
{
	m_prev = d;
}

template <typename T>
void task_deque<T>::set_next(task_deque<T> *d)
{
	m_next = d;
}

template <typename T>
void task_deque<T>::set_victim(task_deque<T> *d)
{
	m_victim = d;
}

template <typename T>
task_deque<T> *task_deque<T>::get_prev()
{
	return m_prev;
}

template <typename T>
task_deque<T> *task_deque<T>::get_next()
{
	return m_next;
}

template <typename T>
task_deque<T> *task_deque<T>::get_victim()
{
	return m_victim;
}

template <typename T>
T *task_deque<T>::put_allocate()
{
	ASSERT(!null(), "Allocate from null deque");

	auto b = load_relaxed(m_bottom);
	return b - 1;
}

template <typename T>
void task_deque<T>::put_commit()
{
	ASSERT(!null(), "Put into null deque");

	auto b = load_relaxed(m_bottom);
	atomic_fence_release();
	store_relaxed(m_bottom, b + 1);
}

template <typename T>
T *task_deque<T>::take(bool *was_stolen)
{
	ASSERT(!null(), "Take from null deque");

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
			*was_stolen = true;
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
T *task_deque<T>::steal(bool *was_empty, bool *was_null)
{
	auto t = load_acquire(m_top);
	if (!t) {
		*was_null = true;
		return nullptr;
	}

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

	inc_relaxed(m_nstolen);

	return t - 1;
}

template <typename T>
bool task_deque<T>::have_stolen() const
{
	return load_relaxed(m_nstolen) > 0;
}

template <typename T>
void task_deque<T>::return_stolen()
{
	ASSERT(m_nstolen > 0, "Decrementing stolen count when there are no stolen tasks");
	dec_relaxed(m_nstolen);
}

template <typename T>
void task_deque<T>::reset()
{
	ASSERT(!have_stolen(), "Resetting when some tasks are stolen");

	store_relaxed(m_top, m_array + 1);
	store_relaxed(m_bottom, m_array + 1);
}

template <typename T>
void task_deque<T>::set_null(bool null)
{
	ASSERT(!have_stolen(), "Changing null state when some tasks are stolen");

	if (null) {
		store_relaxed(m_top, nullptr);
	} else {
		store_relaxed(m_top, m_array + 1);
		store_relaxed(m_bottom, m_array + 1);
	}
}

template <typename T>
bool task_deque<T>::null() const
{
	return load_relaxed(m_top) == nullptr;
}

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_DEQUE_H */
