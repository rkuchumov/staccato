#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

void seq_dfs(size_t depth, size_t breadth, unsigned long *sum)
{
	if (depth == 0) {
		*sum = 1;
		return;
	}

	vector<unsigned long> sums(breadth);

	for (size_t i = 0; i < breadth; ++i)
		seq_dfs(depth - 1, breadth, &sums[i]);

	*sum = 0;
	for (size_t i = 0; i < breadth; ++i)
		*sum += sums[i];
}

int main(int argc, char *argv[])
{
	size_t depth = 8;
	size_t breadth = 8;
	unsigned long answer;

	if (argc >= 3)
		depth = atoi(argv[2]);
	if (argc >= 4)
		breadth = atoi(argv[3]);

	auto start = system_clock::now();

	seq_dfs(depth, breadth, &answer);

	auto stop = system_clock::now();

	cout << "Scheduler:  sequential\n";
	cout << "Benchmark:  dfs\n";
	cout << "Threads:    " << 0 << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << depth << " " << breadth << "\n";
	cout << "Output:     " << answer << "\n";

	return 0;
}
