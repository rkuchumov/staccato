#include <iomanip>
#include "utils.h"
#include "statistics.h"
#include "scheduler.h"

std::chrono::time_point<std::chrono::steady_clock> statistics::start_time;
std::chrono::time_point<std::chrono::steady_clock> statistics::stop_time;

statistics::counter statistics::total = {};
thread_local size_t statistics::me;
statistics::counter *statistics::counters;

void statistics::initialize()
{
	size_t nthreads = scheduler::workers_count + 1;
	counters = new counter[nthreads];
	me = 0;

	start_time = std::chrono::steady_clock::now();
}

void statistics::terminate()
{
	stop_time = std::chrono::steady_clock::now();

	for (size_t i = 0; i < scheduler::workers_count + 1; i++) {
		total.put                   += counters[i].put;
		total.take                  += counters[i].take;
		total.take_failed           += counters[i].take_failed;
		total.single_steal          += counters[i].single_steal;
		total.failed_single_steal   += counters[i].failed_single_steal;
		total.multiple_steal        += counters[i].multiple_steal;
		total.failed_multiple_steal += counters[i].failed_multiple_steal;
		total.resize                += counters[i].resize;
	}

	dump_to_console();
	dump_counters_to_file();
}

void statistics::dump_to_console()
{
	FILE *fp = stderr;
	const static int row_width = 10;
	size_t nthreads = scheduler::workers_count + 1;
	double dt = (double) std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count() / 1000000;

	fprintf(fp, "============= Scheduler Statistics (start) =============\n");
	fprintf(fp, "Threads: %zd\n", nthreads);
	fprintf(fp, "Initial deque size: %d\n", 1 << scheduler::deque_log_size);
	fprintf(fp, "Steal size: %zd\n", scheduler::tasks_per_steal);
	fprintf(fp, "\n");

	fprintf(fp, "Thr  | ");
	fprintf(fp, "%*s | ", row_width, "Put");
	fprintf(fp, "%*s | ", row_width, "Take");
	fprintf(fp, "%*s | ", row_width, "Take.f");
	fprintf(fp, "%*s | ", row_width, "S.Steal");
	fprintf(fp, "%*s | ", row_width, "S.Steal.f");
	fprintf(fp, "%*s | ", row_width, "M.Steal");
	fprintf(fp, "%*s | ", row_width, "M.Steal.f");
	fprintf(fp, "%*s | ", row_width, "Resize");
	fprintf(fp, "\n");

	for (size_t i = 0; i < nthreads; i++) {
		fprintf(fp, "W%-3zd | ", i);
		fprintf(fp, "%*zd | ", row_width, counters[i].put);
		fprintf(fp, "%*zd | ", row_width, counters[i].take);
		fprintf(fp, "%*zd | ", row_width, counters[i].take_failed);
		fprintf(fp, "%*zd | ", row_width, counters[i].single_steal);
		fprintf(fp, "%*zd | ", row_width, counters[i].failed_single_steal);
		fprintf(fp, "%*zd | ", row_width, counters[i].multiple_steal);
		fprintf(fp, "%*zd | ", row_width, counters[i].failed_multiple_steal);
		fprintf(fp, "%*zd | ", row_width, counters[i].resize);
		fprintf(fp, "\n");
	}

	fprintf(fp, "Tot  | ");
	fprintf(fp, "%*zd | ", row_width, total.put);
	fprintf(fp, "%*zd | ", row_width, total.take);
	fprintf(fp, "%*zd | ", row_width, total.take_failed);
	fprintf(fp, "%*zd | ", row_width, total.single_steal);
	fprintf(fp, "%*zd | ", row_width, total.failed_single_steal);
	fprintf(fp, "%*zd | ", row_width, total.multiple_steal);
	fprintf(fp, "%*zd | ", row_width, total.failed_multiple_steal);
	fprintf(fp, "%*zd | ", row_width, total.resize);
	fprintf(fp, "\n");
	fprintf(fp, "\n");


	fprintf(fp, "Elapsed time (sec): %f\n", dt);

	fprintf(fp, "============== Scheduler Statistics (end) ==============\n");
}

void statistics::dump_counters_to_file()
{
	FILE *fp = fopen("scheduler_counters.tab", "w");

	size_t nthreads = scheduler::workers_count + 1;

	fprintf(fp, "Thr\t");
	fprintf(fp, "Put\t");
	fprintf(fp, "Take\t");
	fprintf(fp, "Take.f\t");
	fprintf(fp, "S.Steal\t");
	fprintf(fp, "S.Steal.f\t");
	fprintf(fp, "M.Steal\t");
	fprintf(fp, "M.Steal.f\t");
	fprintf(fp, "Resize\t");
	fprintf(fp, "\n");

	for (size_t i = 0; i < nthreads; i++) {
		fprintf(fp, "W%zd\t", i);
		fprintf(fp, "%zd\t", counters[i].put);
		fprintf(fp, "%zd\t", counters[i].take);
		fprintf(fp, "%zd\t", counters[i].take_failed);
		fprintf(fp, "%zd\t", counters[i].single_steal);
		fprintf(fp, "%zd\t", counters[i].failed_single_steal);
		fprintf(fp, "%zd\t", counters[i].multiple_steal);
		fprintf(fp, "%zd\t", counters[i].failed_multiple_steal);
		fprintf(fp, "%zd\t", counters[i].resize);
		fprintf(fp, "\n");
	}

	fclose(fp);
}

void statistics::set_worker_id(size_t id)
{
	me = id;
}

void statistics::count(event e)
{
	switch (e) {
		case put:
			counters[me].put++;
			break;
		case take:
			counters[me].take++;
			break;
		case take_failed:
			counters[me].take_failed++;
			break;
		case single_steal:
			counters[me].single_steal++;
			break;
		case multiple_steal:
			counters[me].multiple_steal++;
			break;
		case failed_single_steal:
			counters[me].failed_single_steal++;
			break;
		case failed_multiple_steal:
			counters[me].failed_multiple_steal++;
			break;
		default:
			ASSERT(false, "Undefined event: " << e);
	}
}

