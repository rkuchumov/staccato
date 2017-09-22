#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

#include "Counter.hpp"

using namespace std;

class abstract_test
{
public:
	abstract_test(string name_) : name(name_) {
		print_headers();
	}

	virtual ~abstract_test() {

	}

	static size_t iterations;

	static ofstream *out;

	void run() {
		record_t r = {};

		for (size_t i = 0; i < iterations; i++) {
			set_up();

			auto start = std::chrono::steady_clock::now();
			iteration();
			auto end = std::chrono::steady_clock::now();

			r.time += static_cast <double>
				(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1e6;

			r.nfailed += !check();

			r.steals += get_steals();
			r.delay += get_delay();

			break_down();
		}

		appendConsole(r);
		if (out != nullptr)
			appendFile(r);
	}

protected:
	virtual void set_up() = 0;
	virtual void iteration() = 0;
	virtual void break_down() = 0;
	virtual unsigned long get_steals() {
		return 0;
	}
	virtual double get_delay() {
		return 0;
	}
	virtual bool check() {
		return true;
	}

private:
	string name;

	struct record_t
	{
		size_t n;
		size_t nfailed;
		Counter time;
		Counter steals;
		Counter delay;
	};

	static void print_headers() {
		static bool is_printed = false;

		if (is_printed)
			return;

		console_header();
		is_printed = true;
	}

	static void console_header() {
		printf("N:%ld \n\n", iterations);

		printf("%20s | ", "Name");
		printf("%16s | ", "Time");
		printf("%16s | ", "Steals");
		printf("%16s | ", "Delay");
		printf("%5s | ", "Failed");
		printf("\n");
	}

	void appendConsole(const record_t &r) {
		printf("%20s | ", name.c_str());
		printf("%9.5f %5.1f%% | " , r.time.mean()   , r.time.pm(1));
		printf("%9.5f %5.1f%% | " , r.steals.mean() , r.steals.pm(1));
		printf("%9.5f %5.1f%% | " , r.delay.mean()  , r.delay.pm(1));
		printf("%5lu | ", r.nfailed);
		printf("\n");
	}

	void appendFile(const record_t &record) {
		*out << "\"" << name << "\"\t";
		*out << record.n << "\t";
		*out << record.time.mean() << "\t";
		*out << record.time.var() << "\t";
		*out << record.steals.mean() << "\t";
		*out << record.steals.var() << "\t";
		*out << record.delay.mean() << "\t";
		*out << record.delay.var() << "\t";
		*out << endl;
	}
};

size_t abstract_test::iterations = 2;
ofstream *abstract_test::out = nullptr;

#endif /* end of include guard: TEST_SUITE_H */

