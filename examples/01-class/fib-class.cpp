#include <iostream>
#include <atomic>

#include <task_meta.hpp>
#include <scheduler.hpp>

using namespace std;
using namespace staccato;

class FibTask: public task_meta
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

	static void execute(uint8_t *raw) {
		auto task = reinterpret_cast<FibTask *>(raw);
		if (task->n <= 2) {
			*task->sum = 1;
			return;
		}

		cout << "n: " <<  task->n << "\n";

		long x;
		FibTask a(task->n - 1, &x);
		cout << "sub: " <<  a.subtask_count << "\n";

		task->spawn(reinterpret_cast<uint8_t *> (&a));

		// long y;
		// FibTask b(task->n - 2, &y);
		// task->spawn(reinterpret_cast<uint8_t *> (&b));

		task->wait();

		*task->sum = x ;

		return;
	}

private:
	int n;
	long *sum;
};

int main(int argc, char *argv[])
{
	unsigned n = 3;
	long answer; 

	if (argc == 2) {
		n = atoi(argv[1]);
	}


	scheduler::initialize(sizeof(FibTask), FibTask::execute, 1);

	FibTask root(n, &answer);

	scheduler::spawn_and_wait(reinterpret_cast<uint8_t *> (&root));

	scheduler::terminate();

	// cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
