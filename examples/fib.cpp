#include <iostream>

using namespace std;

#include "scheduler.h"

class FibTask: public task
{
public:
	FibTask (int n, long *sum): m_n(n), m_sum(sum) {}
	~FibTask () {};

	void execute() {
		if (m_n < 2) {
			*m_sum = 1;
			return;
		}

		long x, y;

		FibTask *a = new FibTask(m_n - 1, &x);
		FibTask *b = new FibTask(m_n - 2, &y);

		spawn(a);
		spawn(b);

		wait_for_all();

		*m_sum = x + y;

		return;
	}

private:
	int m_n;
	long *m_sum;
};

long fib_seq(int n)
{
    if (n < 2)
    	return 1;

    long x = 1;
    long y = 1;
    long ans = 0;
    for (int i = 2; i <= n; i++) {
        ans = x + y;
        x = y;
        y = ans;
    }

    return ans;
}

int main(int argc, char *argv[])
{
	int n = 30;
	int iter = 1;

	if (argc == 3) {
		n = atoi(argv[1]);
		iter = atoi(argv[2]);

		if (n <= 0 || iter <= 0) {
			cout << "Can't parse arguments\n";
			exit(EXIT_FAILURE);
		}
	} else {
		cout << "Usage:\n   " << argv[0];
		cout << " <N (" << n << ")>";
		cout << " <Iterations (" << iter << ")>\n\n";
	}

	scheduler::initialize(4);

	long ans;
	FibTask *root = new FibTask(n, &ans);

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iter; i++)
		root->execute();

	auto end = std::chrono::steady_clock::now();

	scheduler::terminate();

	long seq = fib_seq(n);
	if (ans == seq)
		cout << "fib(" << n << ") = fib_seq(" << n << ") = " << ans << "\n";
	else
		cout << "Computation error: fib(" << n << ") = " << ans 
			<< "; fib_seq(" << n << ") = " << fib_seq(n) << "\n";

	double dt = (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
	cout << "Iteration time: " << dt / iter << " sec\n";
	cout << "Elapsed time: " << dt << " sec\n";

    return 0;
}
