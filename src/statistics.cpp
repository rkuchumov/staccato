#include "utils.h"
#include "statistics.h"


std::atomic_bool statistics::is_active(false);
std::thread *statistics::stat_thread;

void statistics::initialize()
{
	stat_thread = new std::thread(stat_thread_loop);
	while(!is_active) {};
}

void statistics::terminate()
{
	is_active.store(false);
}

void statistics::stat_thread_loop()
{
	is_active.store(true);

	while(is_active.load()) {
		// TODO!
	}
}

