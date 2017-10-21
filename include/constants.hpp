#ifndef STACCATO_CONSTANTS_H
#define STACCATO_CONSTANTS_H

#ifndef STACCATO_DEBUG
#	define STACCATO_DEBUG 0
#endif // STACCATO_DEBUG

#ifndef STACCATO_STATISTICS
#	define STACCATO_STATISTICS 0
#endif // STACCATO_STATISTICS

#ifndef STACCATO_SAMPLE_DEQUES_SIZES
#	define STACCATO_SAMPLE_DEQUES_SIZES 0
#elif STACCATO_SAMPLE_DEQUES_SIZES == 1
#	define STACCATO_STATISTICS 1
#endif // STACCATO_SAMPLE_DEQUES_SIZES

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


#endif /* end of include guard: STACCATO_CONSTANTS_H */

