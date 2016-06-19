#include <iostream>
#include <fstream>
#include <algorithm>
#include <climits>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <cstring>

using namespace std;

#define EPS 1e-6

size_t interval = 100;
size_t max_diff = 3;

struct sample
{
	double inc;
	double dec;
	double top;
};

ifstream in;

size_t parse_params(int argc, char *argv[]);
void skip_empty(size_t nthreads);
int next_values(size_t *sizes, size_t n);
double my_floor(double x);

typedef unsigned long long ull;

int main(int argc, char *argv[])
{
	unsigned nthreads = parse_params(argc, argv);

	skip_empty(nthreads);
	
	ull **diff = new ull *[nthreads];
	for (size_t i = 0; i < nthreads; i++)
		diff[i] = new ull[2 * max_diff]();

	size_t *next = new size_t[nthreads]();
	size_t *prev = new size_t[nthreads]();
	size_t missed = 0;

	size_t count = 0;

	while (next_values(next, nthreads)) {
		count++;
		for (size_t i = 0; i < nthreads; i++) {
			int delta = next[i] - prev[i];

			if (abs(delta) <= (int) max_diff)
				diff[i][max_diff + delta]++;
			else
				missed++;

			prev[i] = next[i];
		}
	}

	cout << "\n";
	cout << "dt: " << interval << "\n";
	cout << "Intervals: " << count << "\n";
	cout << "Missed: " << missed << " (" << 100.0 * missed / count <<  "%)" << "\n";

	cout << "\nFrequencies:\n        ";
	for (int d = -max_diff; d <= (int)max_diff; d++)
		cout << setw(10) << d << " ";
	cout << "\n";

	for (size_t i = 0; i < nthreads; i++) {
		cout << "Deq." << i << "   ";
		for (int d = -max_diff; d <= (int)max_diff; d++)
			cout << setw(10) << diff[i][max_diff + d] << " ";
		cout << "\n";
	}

	cout << "\n\nProbabilities:\n        ";
	for (int d = -max_diff; d <= (int)max_diff; d++)
		cout << setw(10) << d << " ";
	cout << "\n";

	cout.precision(7);
	for (size_t i = 0; i < nthreads; i++) {
		cout << "Deq." << i << "   ";
		for (int d = -max_diff; d <= (int)max_diff; d++)
			cout << setw(10) << fixed << (double) diff[i][max_diff + d] / count << " ";
		cout << "\n";
	}

	cout << "\n";

	return 0;
}

size_t parse_params(int argc, char *argv[])
{
	if (argc <= 1) {
		cerr << "Usage: " << argv[0] << " FILE [INTERVAL(" << interval << ")]\n";
		exit(EXIT_FAILURE);
	}

	in.open(argv[1]);

	if (argc >= 3)
		interval = atol(argv[2]);
	if (interval == 0) {
		cerr << "Can't parse interval value\n";
		exit(EXIT_FAILURE);
	}

	string s;
	if (!getline(in, s)) {
		cerr << "Can't read header\n";
		exit(EXIT_FAILURE);
	}

	int cols = count(s.begin(), s.end(), '\t') - 1;
	if (cols > 0 && cols % 3 != 0) {
		cerr << "Number of columns should be multiple of 3 (dec, inc, top)\n";
		exit(EXIT_FAILURE);
	}

	return cols / 3;
}

void skip_empty(size_t nthreads)
{
	unsigned long t = 0;
	sample *s = new sample[nthreads];
	
	size_t count = 0;
	streampos pos;

	while (1) {
		if (in.eof()) {
			cerr << "All samples are empty\n";
			exit(EXIT_FAILURE);
		}

		bool is_zero = true;

		pos = in.tellg();

		in >> t;
		for (size_t i = 0; i < nthreads; i++) {
			in >> s[i].inc >> s[i].dec >> s[i].top;

			if (s[i].inc != 0 || s[i].dec != 0 || s[i].top > 1)
				is_zero = false;
		}

		if (!is_zero)
			break;

		count++;
	}
	cout << "Skipped first " << count << " empty samples\n";

	in.seekg(pos);

	delete []s;
}

int next_values(size_t *sizes, size_t n)
{
	static sample *begin   = NULL;
	static sample *current = NULL;
	static sample *end     = NULL;

	static unsigned long end_time   = 0;
	static unsigned long begin_time = 0;

	static size_t n_intervals = 0;
	static size_t processed   = 0;

	if (begin == NULL) {
		begin   = new sample[n];
		end     = new sample[n];
		current = new sample[n];
		
		in >> begin_time;
		for (size_t i = 0; i < n; i ++)
			in >> begin[i].inc >> begin[i].dec >> begin[i].top;
	}

	if (processed == 0) {
		in >> end_time;
		for (size_t i = 0; i < n; i++)
			in >> end[i].inc >> end[i].dec >> end[i].top;

		if (in.eof())
			return 0;

		memcpy(current, begin, n * sizeof(sample));

		n_intervals = (end_time - begin_time) / interval;
	}

	for (size_t i = processed; i < n_intervals; i++) {
		for (size_t j = 0; j < n; j++) {
			current[j].inc += (end[j].inc - begin[j].inc) / n_intervals;
			current[j].dec += (end[j].dec - begin[j].dec) / n_intervals;
			current[j].top += (end[j].top - begin[j].top) / n_intervals;

			sizes[j] = my_floor(current[j].inc - current[j].dec - current[j].top);
		}

		processed++;
		return 1;
	}

	processed = 0;

	if (n_intervals >= 1) {
		memcpy(begin, end, n * sizeof(sample));
		begin_time = end_time;
	} else if (begin_time < end_time){
		cerr << "Time diff between samples (" << (end_time - begin_time)
			<< ") is less than choosen interval (" << interval << ")\n";
	}

	return 1;
}

double my_floor(double x) {
	if (x < 0)
		return 0;

	if (fabs(x - ceil(x)) < EPS)
		return ceil(x);
	else
		return floor(x);
}
