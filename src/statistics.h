#ifndef STATISTICS_H
#define STATISTICS_H

#include <atomic>
#include <thread>

class statistics
{
public:
	static void initialize();
	static void terminate();

private:
	statistics();

	static std::atomic_bool is_active;
	static std::thread *stat_thread;
	static void stat_thread_loop();
};

#endif /* end of include guard: STATISTICS_H */

