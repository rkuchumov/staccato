#ifndef STATISTICS_H
#define STATISTICS_H

#include <atomic>
#include <thread>
#include <chrono>

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

private:
	statistics();

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

	static counter *counters;
	static thread_local size_t me;

	static counter total;
	static void dump_to_console();
	static void dump_counters_to_file();

	static std::chrono::time_point<std::chrono::steady_clock> start_time;
	static std::chrono::time_point<std::chrono::steady_clock> stop_time;

	static unsigned long get_counter_value(counter *c, event e);
	static const char *event_to_str(event e);

#if SAMPLE_DEQUES_SIZES
	static std::thread *stat_thread;
	static void stat_thread_loop();
	static std::atomic_bool stat_thread_is_ready;
	static std::atomic_bool terminate_stat_thread;
	static unsigned long stat_thread_iteratinons;
#endif

};

#endif /* end of include guard: STATISTICS_H */

