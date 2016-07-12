#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

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
		vector<sample_t> samples;

		for (size_t i = 0; i < iterations; i++) {
			sample_t s;

			set_up();

			auto start = std::chrono::steady_clock::now();
			iteration();
			auto end = std::chrono::steady_clock::now();

			s.time = static_cast <double>
				(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1e6;

			s.pass = check();

			break_down();

			samples.push_back(s);
		}

		record_t r = calc_record(samples);
		appendConsole(r);
		if (out != nullptr)
			appendFile(r);
	}


protected:
	virtual void set_up() = 0;
	virtual void iteration() = 0;
	virtual void break_down() = 0;
	virtual bool check() {
		return true;
	}

private:
	string name;

	struct sample_t
	{
		double time;
		bool pass;
	};

	struct record_t
	{
		size_t n;
		double mean;
		double mean_sq;
		double std_dev;
		size_t nfailed;
	};

	record_t calc_record(const vector<sample_t> &samples) {
		record_t r = record_t();
		r.n = samples.size();

		for (const auto &s : samples) {
			r.mean += s.time / r.n;
			r.mean_sq += (s.time * s.time) / (r.n - 1);

			if (!s.pass)
				r.nfailed++;
		}

		double k = static_cast<double> (r.n) / (r.n - 1);
		r.std_dev = sqrt(r.mean_sq - k * r.mean * r.mean);

		return r;
	}

	static void print_headers() {
		static bool is_printed = false;

		if (is_printed)
			return;

		console_header();
		is_printed = true;
	}

	static void console_header() {
		cout << setw(30) << "Name" << " | ";
		cout << setw(4) << "Iter" << " | ";
		cout << setw(13) << "Mean (sec)" << " | ";
		cout << setw(13) << "Std Dev" << " | ";
		cout << setw(13) << "Failed" << " | ";
		cout << "\n";

		cout.setf(ios::fixed, ios::floatfield);
	}

	void appendConsole(const record_t &record) {

		cout << setw(30) << name << " | ";
		cout << setw(4) << record.n << " | ";
		cout.precision(6);
		cout << setw(13) << record.mean << " | ";
		cout << setw(13) << record.std_dev << " | ";
		double p = 100.0f * record.nfailed / record.n;
		if (record.nfailed > 0) {
			cout << setw(13 - 7) << record.nfailed;
			cout.precision(0);
			cout << " (" << setw(3) << p << "%)" << " | ";
		} else {
			cout << setw(13) << record.nfailed << " | ";
		}
		cout << endl;
	}

	void appendFile(const record_t &record) {
		*out << "\"" << name << "\"\t";
		*out << record.n << "\t";
		*out << record.mean << "\t";
		*out << record.std_dev << "\t";
		*out << record.nfailed << "\t";
		*out << endl;
	}
};

size_t abstract_test::iterations = 2;
ofstream *abstract_test::out = nullptr;

#endif /* end of include guard: TEST_SUITE_H */

