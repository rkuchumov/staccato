#include <iostream>
#include <atomic>

#include <task.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace staccato;

class FibTask: public task
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

	void execute() {
		// cout << "n: " << n << "\n";

		if (n <= 2) {
			*sum = 1;
			return;
		}

		long x = 0;
		auto a = new(child()) FibTask(n - 1, &x);
		spawn(a);

		long y;
		auto b = new(child()) FibTask(n - 2, &y);
		spawn(b);

		wait();

		// cout << "x: " << x << " y: " << y << "\n";

		*sum = x + y;

		return;
	}

private:
	int n;
	long *sum;
};

int main(int argc, char *argv[])
{
	unsigned n = 23;
	long answer; 

	if (argc == 2) {
		n = atoi(argv[1]);
	}

	scheduler::initialize(sizeof(FibTask), 4);

	FibTask root(n, &answer);

	scheduler::spawn_and_wait(reinterpret_cast<uint8_t *> (&root));

	scheduler::terminate();

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
