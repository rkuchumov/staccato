#include <iomanip>
#include <vector>
#include <cstring>

#include "utils.h"
#include "statistics.h"
#include "scheduler.h"

namespace staccato
{
namespace internal
{

std::chrono::time_point<std::chrono::steady_clock> statistics::start_time;
std::chrono::time_point<std::chrono::steady_clock> statistics::stop_time;

thread_local size_t statistics::me;
statistics::counter *statistics::counters;
statistics::counter statistics::total = {};

#if STACCATO_SAMPLE_DEQUES_SIZES
statistics::atomic_counter *statistics::atomic_counters;

std::atomic_bool statistics::terminate_stat_thread(false);
std::atomic_bool statistics::stat_thread_is_ready(false);
std::thread *statistics::stat_thread;
unsigned long statistics::stat_thread_iteratinons;
#endif // STACCATO_SAMPLE_DEQUES_SIZES

void statistics::initialize()
{
	size_t nthreads = scheduler::workers_count + 1;
	counters = new counter[nthreads]();
	me = 0;

#if STACCATO_SAMPLE_DEQUES_SIZES
	atomic_counters = new atomic_counter[nthreads];

	stat_thread = new std::thread(stat_thread_loop);
	while (!stat_thread_is_ready) {};
#endif // STACCATO_SAMPLE_DEQUES_SIZES

	start_time = std::chrono::steady_clock::now();
}

#if STACCATO_SAMPLE_DEQUES_SIZES
void statistics::stat_thread_loop()
{
	size_t nthreads = scheduler::workers_count + 1;

	sample *v = new sample[max_samples];
	for (size_t i = 0; i < max_samples; i++)
		v[i].counters = new unsigned int[3 * nthreads]();
	// XXX: I have no time do it nice way :)

	stat_thread_is_ready = true;
	while (!scheduler::is_active) { } // Wait until schedulter starts

	bool is_limit_reached = false;

	while (!terminate_stat_thread) {
		size_t id = stat_thread_iteratinons;
		if (id >= max_samples) {
			is_limit_reached = true;
			id = max_samples - 1; // So that thread does same work and won't mess up with stat.
		}

		for (size_t i = 0; i < nthreads; i++) {
			v[id].counters[3 * i] = load_consume(atomic_counters[i].bottom_inc);
			v[id].counters[3 * i + 1] = load_consume(atomic_counters[i].bottom_dec);
			v[id].counters[3 * i + 2] = scheduler::pool[i].get_top() - 1;
		}

		v[id].time = rdtsc();

		stat_thread_iteratinons++;
	}

	if (is_limit_reached)
		std::cerr << "Sample limit is reached (" << max_samples << "), " <<
			"ignoring the rest\n";

	FILE *fp = fopen("scheduler_deque_sizes.tab", "w");
	fprintf(fp, "Time\t");
	for (size_t i = 0; i < nthreads; i++) {
		fprintf(fp, "Deque%zd.b_inc\t", i);
		fprintf(fp, "Deque%zd.b_dec\t", i);
		fprintf(fp, "Deque%zd.t_inc\t", i);
	}
	fprintf(fp, "\n");

	size_t last = is_limit_reached ? max_samples : stat_thread_iteratinons;
	for (size_t i = 0; i < last; i++) {
		fprintf(fp, "%lu\t", v[i].time);

		for (size_t j = 0; j < 3 * nthreads; j++)
			fprintf(fp, "%u\t", v[i].counters[j]);
		fprintf(fp, "\n");
	}
}
#endif // STACCATO_SAMPLE_DEQUES_SIZES

void statistics::terminate()
{
	stop_time = std::chrono::steady_clock::now();

#if STACCATO_SAMPLE_DEQUES_SIZES
	terminate_stat_thread = true;
	stat_thread->join();
#endif // STACCATO_SAMPLE_DEQUES_SIZES

	total = {};

	for (size_t i = 0; i < scheduler::workers_count + 1; i++) {
		total.put                   += counters[i].put;
		total.take                  += counters[i].take;
		total.take_failed           += counters[i].take_failed;
		total.single_steal          += counters[i].single_steal;
		total.single_steal_failed   += counters[i].single_steal_failed;
		total.multiple_steal        += counters[i].multiple_steal;
		total.multiple_steal_failed += counters[i].multiple_steal_failed;
		total.resize                += counters[i].resize;
	}

	dump_to_console();
	// dump_counters_to_file();
}

const char *statistics::event_to_str(unsigned e)
{
	switch (e) {
		case noop:
			return "Noop"; 
		case take:
			return "Take";
		case put:
			return "Put";
		case single_steal:
			return "S.Streal";
		case multiple_steal:
			return "M.Streal";
		case take_failed:
			return "Take.f";
		case single_steal_failed:
			return "S.Streal.f";
		case multiple_steal_failed:
			return "M.Streal.f";
		case resize:
			return "Resize";
		default:
			break;
	}

	ASSERT(false, "String name for event " << e << " is not set");
	return nullptr;
}

unsigned long statistics::get_counter_value(counter *c, unsigned e) {
	switch (e) {
		case take:
			return c->take;
		case put:
			return c->put;
		case single_steal:
			return c->single_steal;
		case multiple_steal:
			return c->multiple_steal;
		case take_failed:
			return c->take_failed;
		case single_steal_failed:
			return c->single_steal_failed;
		case multiple_steal_failed:
			return c->multiple_steal_failed;
		case resize:
			return c->resize;
		default:
			break;
	}

	ASSERT(false, "There is no counter for " << e << " event");
	return 0;
}

void statistics::dump_to_console()
{
	FILE *fp = stderr;
	const static int row_width = 10;
	size_t nthreads = scheduler::workers_count + 1;
	double dt = static_cast<double> 
		(std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count())
		/ 1000000;

	fprintf(fp, "============= Scheduler Statistics (start) =============\n");
	fprintf(fp, "Threads: %zd\n", nthreads);
	fprintf(fp, "Initial deque size: %d\n", 1 << scheduler::deque_log_size);
	fprintf(fp, "Steal size: %zd\n", scheduler::tasks_per_steal);
	fprintf(fp, "\n");

	fprintf(fp, "Thr  | ");
	for (size_t i = put; i <= resize; i++)
		fprintf(fp, "%*s | ", row_width, event_to_str(i));
	fprintf(fp, "\n");

	for (size_t i = 0; i < nthreads; i++) {
		fprintf(fp, "W%-3zd | ", i);

		for (size_t e = put; e <= resize; e++)
			fprintf(fp, "%*lu | ", row_width, get_counter_value(&counters[i], e));

		fprintf(fp, "\n");
	}

	fprintf(fp, "Tot  | ");
	for (size_t e = put; e <= resize; e++)
		fprintf(fp, "%*lu | ", row_width, get_counter_value(&total, e));
	fprintf(fp, "\n\n");

#if STACCATO_SAMPLE_DEQUES_SIZES
	fprintf(fp, "Stat thread iterations: %lu\n", stat_thread_iteratinons);

	for (size_t i = 0; i < nthreads; i++) {
		unsigned events = counters[i].put +
			counters[i].take + counters[i].take_failed +
			counters[i].single_steal + counters[i].single_steal_failed;

		fprintf(fp, "Iterations per operation (thread %zd): %f\n", i,
				(double) stat_thread_iteratinons / events);
	}
	fprintf(fp, "\n");
#endif // STACCATO_SAMPLE_DEQUES_SIZES

	fprintf(fp, "Elapsed time (sec): %f\n\n", dt);

	fprintf(fp, "============== Scheduler Statistics (end) ==============\n");
}

void statistics::dump_counters_to_file()
{
	FILE *fp = fopen("scheduler_counters.tab", "wa");

	size_t nthreads = scheduler::workers_count + 1;

	for (size_t i = noop; i <= resize; i++)
		fprintf(fp, "%s\t", event_to_str(i));
	fprintf(fp, "\n");

	for (size_t i = 0; i < nthreads; i++) {
		fprintf(fp, "W%zd\t", i);
		for (size_t e = put; e <= resize; e++)
			fprintf(fp, "%zd\t", get_counter_value(&counters[i], e));
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

#if STACCATO_SAMPLE_DEQUES_SIZES
			inc_relaxed(atomic_counters[me].bottom_inc);
#endif // STACCATO_SAMPLE_DEQUES_SIZES
			break;
		case take:
			counters[me].take++;

#if STACCATO_SAMPLE_DEQUES_SIZES
			inc_relaxed(atomic_counters[me].bottom_dec);
#endif // STACCATO_SAMPLE_DEQUES_SIZES
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
		case single_steal_failed:
			counters[me].single_steal_failed++;
			break;
		case multiple_steal_failed:
			counters[me].multiple_steal_failed++;
			break;
		case resize:
			counters[me].resize++;
			break;
		default:
			ASSERT(false, "Undefined event: " << e);
	}
}

statistics::counter statistics::get_counters()
{
	counter r = {};
	for (size_t i = 0; i < scheduler::workers_count + 1; i++) {
		r.put                   += counters[i].put;
		r.take                  += counters[i].take;
		r.take_failed           += counters[i].take_failed;
		r.single_steal          += counters[i].single_steal;
		r.single_steal_failed   += counters[i].single_steal_failed;
		r.multiple_steal        += counters[i].multiple_steal;
		r.multiple_steal_failed += counters[i].multiple_steal_failed;
		r.resize                += counters[i].resize;
	}
	return r;
}

} // namespace internal
} // namespace stacccato
