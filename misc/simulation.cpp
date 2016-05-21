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
int nprocessors        = 4;
const double lambdas[] = {0.5, 0.7, 0.8, 0.9, 0.95, 0.999};
const double mu        = 1;

const int     steal_size_from = 1;
const int     steal_size_to   = 20;

const double  max_time        = 3000;
const int     iterations      = 1000;

// Counters:
double   total_delay      =   0;
long     tasks_executed   =   0;
long     total_arrivals   =   0;
double   total_steals     =   0;
double   total_steals_sq  =   0;
double   queue_size       =   0;

long   run_steals = 0;

void print_header();
void reset_counters();
void print_counters();

#if 0
#define D(x) (x)
#else
#define D(x)
#endif

#define LENGTH(array) static_cast<size_t> (sizeof(array) / sizeof(array[0]))

inline double poisson(double lambda);
inline double unif(double a, double b);
inline int d_unif(int s);

double now = 0;
double dt  = 1;
double lambda = 0.5;
int      steal_size    =   1;

struct task_t
{
	task_t() {
		static int last_id = 0;

		id = last_id++;
		
		arrive_time = now;
		service_time = poisson(mu);
		start_time = -1;

		total_arrivals++;
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
	}

	void execute(task_t *t) {
		assert(current_task == NULL);
		current_task = t;
		t->start_time = now;
		total_delay += now - t->arrive_time;
	}

	void finish() {
		assert(current_task != NULL);

		current_task->print();

		delete current_task;
		current_task = NULL;

		tasks_executed++;
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

	task_t *current_task;
	double next_arrival;
	deque<task_t *> queue;
};

processor_t *processors;

task_t *steal_task(int thief);
double min_arrival();
double min_departure();

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
			reset_counters();
			for (int iter = 0; iter < iterations; iter++) {

				run_steals = 0;

				run();

				total_steals += static_cast<double> (run_steals) / iterations;
				total_steals_sq += static_cast<double> (run_steals * run_steals) / iterations;

			}
			print_counters();

		}
	}

	cout << "\n";


	return 0;
}

void run()
{
	now = 0;

	for (int i = 0; i < nprocessors; i++)
		processors[i].reset();

	bool is_interval  = true;
	bool is_arrival   = true;
	bool is_departure = false;

	double next_arrival   = 0;
	double next_departure = 0;
	double next_interval  = 0;

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

		if (is_interval) {
			if (now >= max_time)
				break;

			next_interval += dt;

			for (int i = 0; i < nprocessors; i++)
				queue_size += static_cast <double> (processors[i].queue.size()) /
					(nprocessors * max_time);

			D(cout << "\n---------------------- " << now << " ----------------------------\n");
		}

		is_arrival   = false;
		is_departure = false;
		is_interval  = false;

		next_arrival = min_arrival();
		next_departure = min_departure();

		if (next_interval <= min(next_arrival, next_departure)) {
			now = next_interval;
			is_interval = true;
		}

		if (next_arrival <= min(next_interval, next_departure)) {
			now = next_arrival;
			is_arrival = true;
		}

		if (next_departure <= min(next_interval, next_arrival)) {
			now = next_departure;
			is_departure = true;
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

	run_steals++;

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

inline double poisson(double lambda) 
{
	double r = static_cast<double> (rand()) / RAND_MAX;
	return -log(r) / lambda;
}

inline double unif(double a, double b)
{
	double r = static_cast<double> (rand()) / RAND_MAX;
	return (b - a) * r + a;
}

inline int d_unif(int s)
{
	double r = static_cast<double> (rand()) / RAND_MAX;
	return floor(r * s) + 1;
}

void reset_counters()
{
	total_delay     = 0;
	tasks_executed  = 0;
	total_arrivals  = 0;
	total_steals    = 0;
	total_steals_sq = 0;
	queue_size      = 0;
}

void print_header()
{
	cout << "lambda\t";
	cout << "k\t";
	cout << "delay\t";
	cout << "steals\t";
	cout << "steals.sd\t";
	cout << "queue\t";
	cout << "\n";
}

void print_counters()
{
	tasks_executed /= iterations;
	queue_size /= iterations;
	total_delay /= (tasks_executed * iterations);

	double steals_sd = sqrt(total_steals_sq - total_steals * total_steals);

	cout.precision(5);

	cout << lambda << "\t";
	cout << steal_size << "\t";

	cout << total_delay << "\t";
	cout << total_steals << "\t";
	cout << steals_sd << "\t";
	cout << queue_size << "\t";

	cout << endl;
}
