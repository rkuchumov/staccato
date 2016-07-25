#ifndef STACCATO_STATISTICS_H
#define STACCATO_STATISTICS_H

#include <atomic>
#include <thread>
#include <chrono>

#include "constants.h"

namespace staccato
{
namespace internal
{

class statistics
{
public:
	static void initialize();
	static void terminate();

	static void set_worker_id(size_t id);

	enum event { 
		noop                  = 0,
		put                   = 1,
		take                  = 2,
		single_steal          = 3,
		multiple_steal        = 4,
		take_failed           = 5,
		single_steal_failed   = 6,
		multiple_steal_failed = 7,
		resize                = 8,
	};

	static void count(event e);

	struct counter
	{
		unsigned long put;
		unsigned long take;
		unsigned long take_failed;
		unsigned long single_steal;
		unsigned long multiple_steal;
		unsigned long single_steal_failed;
		unsigned long multiple_steal_failed;
		unsigned long resize;
	};

	static counter get_counters();

private:
	statistics();


	struct atomic_counter
	{
		std::atomic_ulong bottom_inc;
		std::atomic_ulong bottom_dec;
	};

	static counter *counters;
	static STACCATO_TLS size_t me;

	static counter total;
	static void dump_to_console();
	static void dump_counters_to_file();

	static std::chrono::time_point<std::chrono::steady_clock> start_time;
	static std::chrono::time_point<std::chrono::steady_clock> stop_time;

	static unsigned long get_counter_value(counter *c, unsigned e);
	static const char *event_to_str(unsigned e);

#if STACCATO_SAMPLE_DEQUES_SIZES
	static atomic_counter *atomic_counters;

	struct sample
	{
		uint64_t time;
		unsigned int *counters;
	};

	static const size_t max_samples = 1e7;
	static std::thread *stat_thread;
	static void stat_thread_loop();
	static std::atomic_bool stat_thread_is_ready;
	static std::atomic_bool terminate_stat_thread;
	static unsigned long stat_thread_iteratinons;
#endif // STACCATO_SAMPLE_DEQUES_SIZES

};

} // namespace internal
} // namespace stacccato

#endif /* end of include guard: STACCATO_STATISTICS_H */

