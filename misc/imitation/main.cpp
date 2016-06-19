#include <iostream> 
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <assert.h>
#include <iomanip>
#include <float.h>
#include <vector>

using namespace std;

// Parameters:
int nprocessors          = 4;
const double lambdas[]   = {0.5, 0.7, 0.8, 0.9, 0.95, 0.999};
const double mu          = 1;
const double steal_sleep = 0.05;

const int     steal_size_from = 1;
const int     steal_size_to   = 20;

const double  max_time        = 1000;
const int     iterations      = 300;

// Debug:
#if 0
#define D(x) (x)
#else
#define D(x)
#endif

struct counters_t
{
	double  steals;
	double  delay;
	double  queue;
	double  executed;
	double  arrivals;
	double  empty;
};

counters_t iter_counters;
counters_t counters;
counters_t counters_square;

void print_header();
void reset_counters();
void print_counters();
void update_counters();

// Current iteration values:
double now          = 0;
double dt           = 1;
double lambda       = 0.5;
int    steal_size   = 1;

#define LENGTH(array) static_cast<size_t> (sizeof(array) / sizeof(array[0]))
inline double poisson(double lambda);

struct task_t
{
	task_t() {
		static int last_id = 0;

		id = last_id++;
		
		arrive_time = now;
		service_time = poisson(mu);
		start_time = -1;

		iter_counters.arrivals++;
	}

	void print() {
		D(cout << "i:" << setw(3) << id << " ");
		D(cout << "t:" << setw(5) << fixed << arrive_time << " ");
		D(cout << "S:" << setw(5) << fixed << service_time << " ");
		D(cout << "| ");
	}

	int id;
	double arrive_time;
	double service_time;
	double start_time;
};

struct processor_t
{
	void reset() {
		current_task = NULL;
		next_arrival = 0;
		queue.clear();
		empty_since = 0;
	}

	void execute(task_t *t) {
		assert(current_task == NULL);
		current_task = t;
		t->start_time = now;
		iter_counters.delay += now - t->arrive_time;

		if (empty_since != 0)
			iter_counters.empty += (now - empty_since);
		empty_since = 0;
	}

	void finish() {
		assert(current_task != NULL);

		current_task->print();

		delete current_task;
		current_task = NULL;

		iter_counters.executed++;

		if (queue.size() == 0)
			empty_since = now;
	}

	void put(task_t *t) {
		queue.push_front(t);
	}

	task_t *take() {
		assert(queue.size() > 0);
		task_t *t = queue.front();
		queue.pop_front();
		return t;
	}

	task_t *steal() {
		assert(queue.size() > 0);
		task_t *t = queue.back();
		queue.pop_back();
		return t;
	}

	bool is_departure() {
		if (current_task == NULL)
			return false;

		if (current_task->start_time < 0)
			return false;

		if (current_task->start_time + current_task->service_time > now)
			return false;

		return true;
	}

	double empty_since;
	task_t *current_task;
	double next_arrival;
	deque<task_t *> queue;
};

processor_t *processors;

task_t *steal_task(int thief);
double min_arrival();
double min_departure();
double min_steal();

void run();

int main(int argc, char *argv[]) {
	srand(time(NULL));

	if (argc == 2) {

		nprocessors = atoi(argv[1]);
		if (nprocessors == 0) {
			cerr << "Usage: " << argv[0] << " <N_Processors>\n";
			exit(EXIT_FAILURE);
		}
	}

	processors = new processor_t[nprocessors]();

	print_header();

	for (size_t l = 0; l < LENGTH(lambdas); l++) {
		lambda = lambdas[l];

		for (steal_size = steal_size_from; steal_size <= steal_size_to; steal_size++) {
			counters = (counters_t) {};
			counters_square = (counters_t) {};

			for (int iter = 0; iter < iterations; iter++) {
				iter_counters = (counters_t) {};

				run();

				update_counters();
			}
			print_counters();
		}
	}

	cout << "\n";

	return 0;
}

void run() {
	now = 0;

	for (int i = 0; i < nprocessors; i++)
		processors[i].reset();

	bool is_interval  = true;
	bool is_arrival   = true;
	bool is_departure = false;
	bool is_steal     = false;

	double next_arrival   = 0;
	double next_departure = 0;
	double next_interval  = 0;
	double next_steal     = 0;

	while (1) {
		if (is_arrival) {
			for (int j = 0; j < nprocessors; j++) {
				if (processors[j].next_arrival != now)
					continue;

				task_t *t = new task_t();
				D(cout << "inc(" << j << "):   ");
				t->print();

				if (processors[j].current_task != NULL) {
					D(cout << "pend");
					processors[j].put(t);
				} else {
					assert(processors[j].queue.size() == 0);
					D(cout << "exec");
					processors[j].execute(t);
				}

				processors[j].next_arrival = now + poisson(lambda);
				D(cout << " | next:" << processors[j].next_arrival << "\n");
			}
		}

		if (is_departure) {
			for (int j = 0; j < nprocessors; j++) {
				if (!processors[j].is_departure())
					continue;

				D(cout << "dep("<< j << "):   ");
				processors[j].finish();

				if (processors[j].queue.size() == 0) {
					D(cout << "stea |");
					task_t *t = steal_task(j);
					if (t) {
						D(cout << " success\n");
						processors[j].execute(t);
					} else {
						D(cout << " fail\n");
					}
				} else {
					task_t *t = processors[j].take();
					D(cout << "next | " << t->id << "\n");
					processors[j].execute(t);
				}
			}
		}

		if (is_steal) {
			for (int j = 0; j < nprocessors; j++) {
				if (processors[j].current_task != NULL)
					continue;

				D(cout << "ste(" << j << "):   ");

				task_t *t = steal_task(j);
				if (t) {
					t->print();
					D(cout << "\n");
					processors[j].execute(t);
				} else {
					D(cout << "fail\n");
				}
			}
		}

		if (is_interval) {
			if (now >= max_time)
				break;

			next_interval += dt;

			for (int i = 0; i < nprocessors; i++)
				iter_counters.queue += static_cast <double> (processors[i].queue.size()) /
					(nprocessors * max_time);

			D(cout << "\n---------------------- " << now << " ----------------------------\n");
		}

		is_arrival   = false;
		is_departure = false;
		is_interval  = false;
		is_steal     = false;

		next_arrival   = min_arrival();
		next_departure = min_departure();
		next_steal     = min_steal();

		if (next_interval <= min(next_arrival, min(next_departure, next_steal))) {
			now = next_interval;
			is_interval = true;
		}

		if (next_arrival <= min(next_interval, min(next_departure, next_steal))) {
			now = next_arrival;
			is_arrival = true;
		}

		if (next_departure <= min(next_interval, min(next_arrival, next_steal))) {
			now = next_departure;
			is_departure = true;
		}

		if (next_steal <= min(next_arrival, min(next_departure, next_interval))) {
			now = next_steal;
			is_steal = true;
		}
	}

	D(cout << "\n=============================================================\n");
}

task_t *steal_task(int thief)
{
	vector<int> victims;

	for (int j = 0; j < nprocessors; j++) {
		if (processors[j].queue.size() >= 1) {
			victims.push_back(j);
		}
	}

	iter_counters.steals++;

	if (victims.size() == 0)
		return NULL;

	int victim = victims[rand() % victims.size()];

	if (processors[victim].queue.size() < static_cast<size_t> (steal_size))
		return processors[victim].steal();

	for (int i = 1; i < steal_size; i++) {
		task_t *t = processors[victim].steal();
		processors[thief].put(t);
	}

	return processors[victim].steal();
}

double min_arrival()
{
	double m = FLT_MAX;
	for (int j = 0; j < nprocessors; j++) {
		if (m > processors[j].next_arrival)
			m = processors[j].next_arrival;
	}
	assert(m != FLT_MAX);

	return m;
}

double min_departure()
{
	double m = FLT_MAX;

	for (int j = 0; j < nprocessors; j++) {
		if (processors[j].current_task == NULL)
			continue;
		if (processors[j].current_task->start_time < 0)
			continue;
		double stop =
			processors[j].current_task->start_time +
			processors[j].current_task->service_time;
		if (stop < m)
			m = stop;
	}

	return m;
}

double min_steal()
{
	for (int j = 0; j < nprocessors; j++) {
		if (processors[j].current_task != NULL)
			continue;

		return now + steal_sleep;
	}

	return FLT_MAX;
}

inline double poisson(double lambda) 
{
	double r = static_cast<double> (rand()) / RAND_MAX;
	return -log(r) / lambda;
}

void update_counters()
{
	counters.steals += iter_counters.steals / iterations;
	counters_square.steals
		+= (iter_counters.steals * iter_counters.steals) / (iterations - 1);

	counters.delay += iter_counters.delay / iterations;
	counters_square.delay
		+= (iter_counters.delay * iter_counters.delay) / (iterations - 1);

	counters.queue += iter_counters.queue / iterations;
	counters_square.queue
		+= (iter_counters.queue * iter_counters.queue) / (iterations - 1);

	counters.executed += iter_counters.executed / iterations;
	counters_square.executed 
		+= (iter_counters.executed * iter_counters.executed) / (iterations - 1);

	counters.arrivals += iter_counters.arrivals / iterations;
	counters_square.arrivals
		+= (iter_counters.arrivals * iter_counters.arrivals) / (iterations - 1);

	counters.empty += iter_counters.empty / iterations;
	counters_square.empty
		+= (iter_counters.empty * iter_counters.empty) / (iterations - 1);
}

void print_header()
{
	cout << "rate;   ";
	cout << "k;  ";

	cout << "steals;\t\t"   << "steals.sd;\t";
	cout << "delay;\t\t"    << "delay.sd;\t";
	cout << "queue;\t\t"    << "queue.sd;\t";
	cout << "executed;\t" << "executed.sd;\t";
	cout << "arrivals;\t" << "arrivals.sd;\t";
	cout << "empty;\t\t" << "empty.sd;\t";

	cout << "\n";
}

void print_counters()
{
	double k = static_cast<double> (iterations) / (iterations - 1);

	counters_t std_dev;
	std_dev.steals   = sqrt(counters_square.steals   - k * counters.steals * counters.steals);
	std_dev.delay    = sqrt(counters_square.delay    - k * counters.delay * counters.delay);
	std_dev.queue    = sqrt(counters_square.queue    - k * counters.queue * counters.queue);
	std_dev.executed = sqrt(counters_square.executed - k * counters.executed * counters.executed);
	std_dev.arrivals = sqrt(counters_square.arrivals - k * counters.arrivals * counters.arrivals);
	std_dev.empty    = sqrt(counters_square.empty    - k * counters.empty * counters.empty);

	cout.setf(ios::fixed, ios::floatfield);
	cout.precision(3);
	cout << lambda << "; ";
	cout << setw(2) << steal_size << ";  ";

	cout.precision(6);

	cout << counters.steals   << ";\t" << std_dev.steals   << ";\t";
	cout << counters.delay    << ";\t" << std_dev.delay    << ";\t";
	cout << counters.queue    << ";\t" << std_dev.queue    << ";\t";
	cout << counters.executed << ";\t" << std_dev.executed << ";\t";
	cout << counters.arrivals << ";\t" << std_dev.arrivals << ";\t";
	cout << counters.empty    << ";\t" << std_dev.empty    << ";\t";

	cout << endl;
}
