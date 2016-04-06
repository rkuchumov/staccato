#include <iostream>
#include <unistd.h>

using namespace std;

#include "scheduler.h"

int fib_seq(int n)
{
    if (n <= 2) return 1;
    int x = 1;
    int y = 1;
    int ans = 0;
    for (int i = 2; i <= n; i++) {
        ans = x + y;
        x = y;
        y = ans;
    }
    return ans;
}

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

		set_subtask_count(2);
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

int main()
{
	scheduler::initialize(4);

	long ans;
	int n = 35;

	FibTask *root = new FibTask(n, &ans);
	root->execute();

    cerr << "\nfib(" << n << ") = " << ans << "\n";
    if (ans != fib_seq(n))
        cerr << "WRONG correct: " << fib_seq(n) << "\n";

	scheduler::terminate();

    return 0;
}
