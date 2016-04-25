#include <iostream>
#include <fstream>
#include <chrono>

#include "taskexec.h"

#include "fib.h"
#include "custom.h"
#include "knapsack.h"
#include "matmult.h"
#include "mergesort.h"

using namespace std;

int iterations = 3;

void parse_args(int argc, char *argv[]);

int main()
{
	char stat_file[128];
	snprintf(stat_file, sizeof(stat_file), "results.txt");
	ofstream out(stat_file);

	{ FibTaskExec(35, 4, iterations, 1).test(out); }

	{ CustomTaskExec(300, 3, 4, iterations, 1).test(out); }

	{ KnapsackTaskExec(26, 4, iterations, 1).test(out); }

	{ MatmultExec(8, 4, iterations, 1).test(out); }

	{ MergeSortExec(24, false, 4, iterations, 1).test(out); }

	{ MergeSortExec(24, true, 4, iterations, 1).test(out); }

    return 0;
}
