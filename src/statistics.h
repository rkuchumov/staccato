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
		put                   = 1,
		take                  = 2,
		take_failed           = 3,
		single_steal          = 4,
		multiple_steal        = 5,
		failed_single_steal   = 6,
		failed_multiple_steal = 7,
		resize                = 8
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
		unsigned long failed_single_steal;
		unsigned long failed_multiple_steal;
		unsigned long resize;
	};

	static counter *counters;
	static thread_local size_t me;


	static counter total;
	static void dump_to_console();
	static void dump_counters_to_file();


	static std::chrono::time_point<std::chrono::steady_clock> start_time;
	static std::chrono::time_point<std::chrono::steady_clock> stop_time;
};

#endif /* end of include guard: STATISTICS_H */

