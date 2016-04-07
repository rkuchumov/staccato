#include <iostream>
#include <unistd.h>

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

	for (int i = 0; i < iter; i++)
		root->execute();

	scheduler::terminate();

    cerr << "fib(" << n << ") = " << ans << "\n";

    return 0;
}
