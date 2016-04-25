#ifndef TASKEXEC_H
#define TASKEXEC_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>

class TaskExec
{
public:
	TaskExec(size_t nthreads_, size_t iterations_, size_t steal_size_):
		nthreads(nthreads_), iterations(iterations_), steal_size(steal_size_)
	{ }

	virtual ~TaskExec() { }

	virtual void run() = 0;

	virtual const char *test_name() = 0;

	virtual bool check() {
		return true;
	}

	void test(std::ofstream &out) {
		out << test_name() << "_" << nthreads << "_" << steal_size << "\t";
		std::cout << "=================================\n";
		std::cout << test_name() << " threads:" << nthreads << "; steal:" << steal_size << "\n";

		double mean = 0;
		double mean_sq = 0;

		for (size_t i = 0; i < iterations; i++) {
			auto start = std::chrono::steady_clock::now();
			run();
			auto end = std::chrono::steady_clock::now();

			if (!check()) {
				std::cerr << "Computation error\n";
				exit(EXIT_FAILURE);
			}

			double dt = static_cast <double> (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1000000;

			out << dt << "\t";

			mean += dt;
			mean_sq += dt*dt;
		}

		out << "\n";

		mean /= iterations;
		mean_sq /= iterations;

		double variance = mean_sq - mean * mean;

		std::cout << "Mean: " << mean << " sec\n";
		std::cout << "Variance: " << variance << " sec^2\n";
	}

protected:
	size_t nthreads;
	size_t iterations;
	size_t steal_size;
};

#endif /* end of include guard: TASKEXEC_H */

