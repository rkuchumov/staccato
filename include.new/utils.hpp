#ifndef UTILS_HPP_CSPTFG9B
#define UTILS_HPP_CSPTFG9B

#include <cassert>
#include <cstdint>

#ifndef STACCATO_DEBUG
#	define STACCATO_DEBUG 0
#endif // STACCATO_DEBUG

#if !defined(LEVEL1_DCACHE_LINESIZE) || LEVEL1_DCACHE_LINESIZE == 0
#	define STACCATO_CACHE_SIZE 64
#else
#	define STACCATO_CACHE_SIZE LEVEL1_DCACHE_LINESIZE
#endif // LEVEL1_DCACHE_LINESIZE

// XXX: not tested
#if __cplusplus > 199711L
#	define STACCATO_TLS thread_local
#elif __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#	define STACCATO_TLS _Thread_local
#elif defined _WIN32 && ( \
      defined _MSC_VER || \
      defined __ICL || \
      defined __DMC__ || \
      defined __BORLANDC__ )
#	define STACCATO_TLS __declspec(thread) 
#elif defined __GNUC__ || \
      defined __SUNPRO_C || \
      defined __xlC__
#	define STACCATO_TLS __thread
#else
#	define STACCATO_TLS thread_local
#	warning "Cannot define thread_local"
#endif

// XXX: not tested
#if __cplusplus > 199711L
#	define STACCATO_ALIGN alignas(STACCATO_CACHE_SIZE)
#elif defined _WIN32 && ( \
      defined _MSC_VER || \
      defined __ICL || \
      defined __DMC__ || \
      defined __BORLANDC__ )
#	define STACCATO_ALIGN __declspec(align(STACCATO_CACHE_SIZE)) 
#elif defined __GNUC__ || \
      defined __SUNPRO_C || \
      defined __xlC__
#	define STACCATO_ALIGN __attribute__(aligned(STACCATO_CACHE_SIZE))
#else
#	define STACCATO_ALIGN alignas(STACCATO_CACHE_SIZE)
#	warning "Cannot define alignas()"
#endif

#if STACCATO_DEBUG
#   define STACCATO_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << ":" << __LINE__ << "\n" << message  << "\n" << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)
#else
#   define STACCATO_ASSERT(condition, message) do { } while (false)
#endif

// TODO: define inline functions
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

#define dec_relaxed_p(var) (var)->fetch_sub(1, std::memory_order_relaxed)
#define inc_relaxed_p(var) (var)->fetch_add(1, std::memory_order_relaxed)

#define add_relaxed(var, n) (var).fetch_add((n), std::memory_order_relaxed)
#define sub_relaxed(var, n) (var).fetch_sub((n), std::memory_order_relaxed)

#define add_release(var, n) (var).fetch_add((n), std::memory_order_release)
#define sub_release(var, n) (var).fetch_sub((n), std::memory_order_release)

namespace staccato
{
namespace internal
{

inline uint32_t xorshift_rand() {
	STACCATO_TLS static uint32_t x = 2463534242;

	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

inline bool is_pow2(uint64_t x) {
	return x && !(x & (x - 1));
}

inline uint64_t next_pow2(uint64_t x)
{
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return x + 1;
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: UTILS_HPP_CSPTFG9B */
