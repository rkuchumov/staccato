# Work-Stealing Task Scheduler

This is my C++11 implementation of concurrent work-stealing task scheduler. It has only basic scheduling functions (`spawn` and `wait`) and supports weak memory models. 

## How does work-stealing works?
Tasks consists of a sets of instructions that should be executed sequentially. During the execution they can create subtasks which can be run in parallel. Thus all the tasks can be described by directed acyclic graph. 

Scheduler has a set of execution threads with private queues of tasks. When new a subtask is created (by `spawn` call) it is placed in thread's private queue. When task should wait for its subtasks to finish (i.e. `wait` is called), thread starts to execute tasks from its own queue. In case it doesn't have tasks to execute, it steals tasks from queues of others threads.

## What's special about this implementation?

Internal data structures of most work-stealing schedulers are designed with an assumption that they would store memory pointers to task objects. Despite their low overhead, this approach have the following problems:

1. For each task memory allocator should be accessed twice: to allocate task object memory and then to free it, when the task is finished. It brings allocator-related overhead.
2. There's no guarantee that tasks objects will be stored in adjacent memory regions. During tasks graph execution this leads to cache-misses overhead.

In my implementation these problems are addressed by:

1. Designing a deque based data structure that allows to store tasks objects during their execution. So when the task is completed its memory is reused by the following tasks, which eliminates the need for accessing memory manager. Besides, this implementation has lesser lock-contention than traditional work-stealing deques. 
2. Using dedicated memory manager. As deques owner executes tasks in LIFO manner, its memory access follows the same pattern. It allows to use LIFO based memory manager that does not have to provide fragmentation handling and thread-safety thus having lowest overhead possible and stores memory object in consecutive memory.

As a drawback of this approach is that you have to specify the maximum number of subtasks each task can have. For example, for classical tasks of calculating Fibonacci number it's equal to 2. For majority of tasks it's not problem and this number is usually known at developing stage, but I plan bypass this in the future version.  

## How does it compare to other schedulers?

For bench-marking I've used the following tests:

|Name|Dimensions|Type|Description|
|--|--|--|--|
|fib| 42| CPU-bound | Fibonacci number calculation recurrence formula |
|dfs|9^9| CPU-bound | Depth first search of a balanced tree |
|matmul|3500x350| Memory-bound |Cache-aware square matrix multiplication |
|blkmul|4096x4096| Memory-bound |Square block matrix multiplication|

*fib* and *dfs*  benchmarks have relatively small tasks and the majority of CPU time is spend executing the scheduler code. On contrary, *blkmul* and *matmul* benchmarks require more time to execute their tasks than the scheduler code. This can be seen when comparing the execution times of sequential versions with the parallel one, *fib* and *dfs* are far more efficient without a scheduler. This, in turn, means that *fib* and *dfs* are more suited for comparing the overheads of different schedulers. 

For benchmarking I've used a server with two Intel Xeon E5 v4. Each processor has 32K L1i and L2d, 256K L2 and 35 Mb L3 cache, 14 cores and 2 threads per core (56 threads total). Each scheduler was compiled with the same compiler (g++ 7.2.1) and the same optimization level (-O3) under Linux 4.13 (Fedora 27). 

The comparison with Intel TBB (v.4.3), Intel Cilk Plus (7.3) and sequential version show the following results:

<p align="center">
<img
src="https://github.com/rkuchumov/staccato/blob/master/docs/llc-misses.png?raw=true"
alt="llc_misses"
style="max-width:60%" 
>

<img
src="https://github.com/rkuchumov/staccato/blob/master/docs/final.dat-fib.png?raw=true"
alt="fib_benchmark"
style="max-width:60%" 
>

<img
src="https://github.com/rkuchumov/staccato/blob/master/docs/final.dat-dfs.png?raw=true"
alt="dfs_benchmark"
style="max-width:60%" 
>

<img
src="https://github.com/rkuchumov/staccato/blob/master/docs/final.dat-matmul.png?raw=true"
alt="matmul_benchmark"
style="max-width:60%" 
>

<img
src="https://github.com/rkuchumov/staccato/blob/master/docs/final.dat-blkmul.png?raw=true"
alt="blkmul_benchmark"
style="max-width:60%" 
>
</p>

As this implementation attempts to reduce the overhead of internal data structures, the difference is most noticeable in CPU-bound tasks while memory-bound tasks left almost unaffected. 

## Usage

It's header-only library, so no need for compiling it and linking. C++11 with thread support is the only requirement.

Include `staccato/scheduler.hpp` file to use the scheduler in your program.

### Define a task class

Define a class derived from `staccato::task<T>` for your task with `void execute()` method. This method would be executed by the scheduler.

You can spawn substasks during execution of `execute()`. For that:
1. Create new object of your task class over the memory returned by `child()`
2. Call `void spawn(task *t)` to place a new task in thread queue
3. Call `void wait()` to wait for all created subtasks to finish

### Create scheduler object

Create `scheduler<T>` object with specified maximum number of subtasks and a number of threads:

```c++
scheduler<FibTask> sh(2, nthreads);
```

The specified number of execution threads (`nthreads`) will be created. These threads will be removed when the destructor is called. 

### Submit root task for execution

Create an object for the root task over the memory returned by `sh.root()` and submit the task for execution with `sh.spawn()`. To wait until the task is finished call `sh.wait()`:

```c++
sh.spawn(new(sh.root()) FibTask(n, &answer));
sh.wait()
```
 
## Example

```c++
#include <iostream>
#include <thread>

#include <staccato/scheduler.hpp>

using namespace std;
using namespace staccato;

class FibTask: public task<FibTask>
{
public:
	FibTask (int n_, long *sum_): n(n_), sum(sum_)
	{ }

	void execute() {
		if (n <= 2) {
			*sum = 1;
			return;
		}

		long x;
		spawn(new(child()) FibTask(n - 1, &x));
		long y;
		spawn(new(child()) FibTask(n - 2, &y));

		wait();

		*sum = x + y;

		return;
	}

private:
	int n;
	long *sum;
};

int main(int argc, char *argv[])
{
	size_t n = 20;
	long answer; 
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	{
		scheduler<FibTask> sh(2, nthreads);
		sh.spawn(new(sh.root()) FibTask(n, &answer));
		sh.wait();
	}

	cout << "fib(" << n << ") = " << answer << "\n";

	return 0;
}
```

This program calculates 40'th Fibonacci number by recurrence formula. The root task calculates 40'th number by spawning two subtasks for 39'th and 38'th numbers. They, in turn, spawn two subtasks and so on until the first number. When subtasks finish, their sum is calculated and it is stored in parent's task memory.

For more examples check `bencmarks/staccato/` directory.
