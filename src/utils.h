#ifndef STACCATO_UTILS_H
#define STACCATO_UTILS_H

#include <iostream>
#include <cassert>
#include <iostream>
#include <sstream>

#include "constants.h"

#if STACCATO_STATISTICS
#	define COUNT(event) internal::statistics::count(internal::statistics::event);
#else
#	define COUNT(event) do {} while (false);
#endif // STACCATO_STATISTICS

#if STACCATO_DEBUG
#   define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << ":" << __LINE__ << "\n" << message  << "\n" << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

#define load_relaxed(var) (var).load(std::memory_order_relaxed)
#define load_acquire(var) (var).load(std::memory_order_acquire)
#define load_consume(var) (var).load(std::memory_order_consume)

#define store_relaxed(var, x) (var).store((x), std::memory_order_relaxed)
#define store_release(var, x) (var).store((x), std::memory_order_release)

#define atomic_fence_seq_cst() std::atomic_thread_fence(std::memory_order_seq_cst)
#define atomic_fence_release() std::atomic_thread_fence(std::memory_order_release)

#define cas_strong(var, expected, new) (var).compare_exchange_strong((expected), (new), std::memory_order_seq_cst, std::memory_order_relaxed)
#define cas_weak(var, expected, new) (var).compare_exchange_weak((expected), (new), std::memory_order_seq_cst, std::memory_order_relaxed)

#define dec_relaxed(var) (var).fetch_sub(1, std::memory_order_relaxed)
#define inc_relaxed(var) (var).fetch_add(1, std::memory_order_relaxed)

namespace staccato {
namespace internal {

inline uint32_t xorshift_rand() {
	STACCATO_TLS static uint32_t x = 2463534242;

	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

class Debug
{
public:
	Debug(size_t indent = 0)
	: m_printed(false)
	, m_indent(indent)
	{
		m_buffer << "[STACCATO] ";
		for (size_t i = 0; i < m_indent; ++i) {
			m_buffer << "   ";
		}
	}

	~Debug()
	{
		if (!m_printed)
			print();
	}

	void print()
	{
	// TODO: make it thread safe
#if STACCATO_DEBUG
		m_buffer << std::endl;
		std::cerr << m_buffer.str();
#endif
		m_printed = true;
	}

	template <typename T>
	Debug & operator<<(const T &value)
	{
		m_buffer << value;
		return *this;
	}

private:
	std::ostringstream m_buffer;
	bool m_printed;
	size_t m_indent;
};
	
} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: STACCATO_UTILS_H */

