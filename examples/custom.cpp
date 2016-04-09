#include <iostream>
#include <cassert>

using namespace std;

#include "scheduler.h"

struct CustomTask: public task
{
	int depth;
	int breadth;

	CustomTask(int breadth_, int depth_): depth(depth_), breadth(breadth_) {}

	void execute() {
		if (depth == 0)
			return;

		for (int j = 0; j < breadth; j++) {
			CustomTask *t = new CustomTask(breadth, depth - 1);
			spawn(t);
		}

		wait_for_all();
	}
};

int main(int argc, char *argv[])
{
	int b = 256;
	int d = 2;
	int iter = 1;

	if (argc == 4) {
		b = atoi(argv[1]);
		d = atoi(argv[2]);
		iter = atoi(argv[3]);

		if (b <= 0 || d <= 0 || iter <= 0) {
			cout << "Can't parse arguments\n";
			exit(EXIT_FAILURE);
		}
	} else {
		cout << "Usage:\n   " << argv[0];
		cout << " <Breadth (" << b << ")>";
		cout << " <Depth (" << d << ")>";
		cout << " <Iterations (" << iter << ")>\n\n";
	}

	scheduler::initialize();

	CustomTask *root = new CustomTask(b, d);

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iter; i++)
		root->execute();

	auto end = std::chrono::steady_clock::now();

	scheduler::terminate();

	cout << "Done.\n";

    double dt = (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
    cout << "Elapsed time: " << dt << " sec\n";

    return 0;
}
