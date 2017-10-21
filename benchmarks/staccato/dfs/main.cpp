#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include <staccato/task.hpp>
#include <staccato/scheduler.hpp>

using namespace std;
using namespace chrono;
using namespace staccato;

class DFSTask: public task
{
public:
	DFSTask (size_t depth_, size_t breadth_, unsigned long *sum_)
		: depth(depth_)
		, breadth(breadth_)
		, sum(sum_)
	{ 
		*sum = 0;
	}

	void execute() {
		if (depth == 0) {
			*sum = 1;
			return;
		}

		vector<unsigned long> sums(breadth);

		for (size_t i = 0; i < breadth; ++i)
			spawn(new(child()) DFSTask(depth - 1, breadth, &sums[i]));

		wait();

		for (size_t i = 0; i < breadth; ++i)
			*sum += sums[i];

		return;
	}

private:
	size_t depth;
	size_t breadth;
	unsigned long *sum;
};

int main(int argc, char *argv[])
{
	size_t depth = 8;
	size_t breadth = 8;
	unsigned long answer;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		depth = atoi(argv[2]);
	if (argc >= 4)
		breadth = atoi(argv[3]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	auto start = system_clock::now();

	scheduler::initialize(sizeof(DFSTask), nthreads);

	scheduler::spawn(new(scheduler::root()) DFSTask(depth, breadth, &answer));
	scheduler::wait();

	scheduler::terminate();

	auto stop = system_clock::now();

	cout << "Scheduler:  staccato\n";
	cout << "Benchmark:  dfs\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << depth << " " << breadth << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
