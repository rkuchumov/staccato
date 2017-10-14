# Work-Stealing Task Scheduler

This is my C++11 implementation of concurrent work-stealing task scheduler. It has only basic scheduling functions (`spawn` and `wait`) and supports weak memory models. 

## How it works
Tasks consists of a sets of instructions that should be executed sequentially. During the execution they can create subtasks which can be run in parallel. Thus all the tasks can be described by directed acyclic graph. 

Scheduler has a set of execution threads with private queues of tasks. When new subtask is created (by `spawn` call) it is placed in thread's private queue. When task should wait for its subtasks to finish (i.e. `wait` is called), thread starts to execute tasks from its own queue. In case it doesn't have tasks to execute, it steals tasks from queues of others threads.

## Usage

Include `staccato/scheduler.h` file to use the scheduler in your program.

### Initialization and termination

To start scheduler call 

```c++
staccato::scheduler::initialize(size_t nthreads)
```

This method will initialize  the scheduler by creating specified number of threads. If `nthreads` is not set scheduler would create the number of threads equals to the number of processors (`std::thread::hardware_concurrency()`).

To stop the scheduler (and all its threads) call:

```c++
staccato::scheduler::terminate();
```

### Creating tasks

Define a class derived from `staccato::task` for your task with `void execute()` method. This method would be executed by the scheduler.

You can spawn substasks during execution of `execute()`. For that:
1. Create new object of your task class
2. Call `void spawn(task *t)` to place a new task in thread queue
3. Call `void wait()` to wait for all created subtasks to finish

To execute root task call it's `execute()` method. Note that this way root task is always executed in the calling thread i.e. not in parallel.


### Creating a task from lambda function

You can also submit a task for parallel execition by passing a lamda funtion to `scheduler::spawn` function:

```c++
scheduler::initialize();

scheduler::spawn([&]() {
	// ...
});

scheduler::wait();

scheduler::terminate();
```

## Example

```c++
#include <iostream>
#include <staccato/task.hpp>
#include <staccato/scheduler.hpp>

using namespace std;
using namespace staccato;

class FibTask: public task {
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_) { }

	void execute() {
		if (n <= 2) {
			*sum = 1;
			return;
		}

		long x, y;
		FibTask *a = new FibTask(n - 1, &x);
		FibTask *b = new FibTask(n - 2, &y);

		spawn(a);
		spawn(b);

		wait();

		delete a;
		delete b;

		*sum = x + y;
		return;
	}

private:
	int n;
	long *sum;
};

int main(int argc, char *argv[]) {
	unsigned n = 40;
	long answer;

	scheduler::initialize();

	FibTask root(n, &answer);
	scheduler::spawn(&root);
	scheduler::wait();

	scheduler::terminate();

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
```

This program calculates 40'th Fibonacci number by recurrence formula. The root task calculates 40'th number by spawning two subtasks for 39'th and 38'th numbers. They, in turn, spawn two subtasks and so on until the first number. When subtasks finish, their sum is calculated and it is stored in parent's task memory.

More examples can be found in `examples/` and `benchmarks/staccato/` directories.

