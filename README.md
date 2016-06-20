#Work-Stealing Task Scheduler

This is my C++11 implementation of concurrent work-stealing task scheduler. It has the following features:

 1. Has only basic scheduling functions (`spawn` and `wait_for_all`)
 2. Supports weak memory model
 2. Allows to change the number of tasks removed per single steal
 3. Allows to collect statistical information about scheduler behavior

##How it works
Tasks are a sets of instructions that should be executed sequentially. During the execution they can create subtasks which can be run in parallel. Thus all the tasks can be described by directed acyclic graph. 

Scheduler has a set of execution threads with private queues of tasks. When new subtask is created (by `spawn` call) it is placed in thread's private queue. When task should wait for its subtasks to finish (i.e. `wait_for_all` is called), thread starts to execute tasks from its own queue. In case it doesn't have tasks to execute, it steals tasks from queues of others threads.

##Usage

### Compilation

To compile scheduler library you'll need a compiler with C++11 support (e.g. gcc 5.2+).
From `build` directory run one of the following commands:

 - `make` -- to compile release version
 - `make cfg=debug` -- to compile debug version that 
 - `make cfg=statistics` -- to compile version that shows statistics (operations counters, queues sizes etc)

These commands will create `release`, `debug` or `statistics` directory with object files and `libstaccato.so` shared library file that you can link to your program.

To compile statistics version that collects queues sizes during program execution add `-DSTACCATO_SAMPLE_DEQUES_SIZES=1` flag either by modifying `Makefile` or thought `make` arguments.

### Linking
To use scheduler headers in your program add the path to `include` directory to your include path, for example by using `-I<path>` flag in gcc.

To link `libstaccato.so` add path to this file in `LD_LIBRARY_PATH` variable or use `-Wl,-rpath=<path>` flag in gcc.

In case you compiled scheduler in debug or statistics mode add the following flags to the compiler or before including headers:

 - `-DSTACCATO_DEBUG=1 -DSTACCATO_STATISTICS=1` for debug mode
 - `-DSTACCATO_STATISTICS=1` for statistics mode
 - `-DSTACCATO_STATISTICS=1 -DSTACCATO_SAMPLE_DEQUES_SIZES=1` for statistics mode that collects queues sizes

One of the possible ways to link scheduler you can find in `examples/Makefile`

###Interface

Include `scheduler.h` file to use the scheduler in your program.

####Initialization and termination
To start scheduler call 
```
staccato::scheduler::initialize(size_t nthreads)
```
This method will initialize  the scheduler by creating specified number of threads. If `nthreads` is not set scheduler would create the number of threads equals to the number of processors (hardware_concurrency).

You can also set default queue size and the number of tasks removed per single steal operation by setting the following variables before calling `initialize()`:
```
staccato::scheduler::deque_log_size = 11 // = 2^11 = 2048
staccato::scheduler::tasks_per_steal = 3
```

To stop the scheduler (and all its threads) call:
```
staccato::scheduler::terminate();
```

####Creating tasks

Define a class derived from `staccato::task` for your task with `void execute()` method. This method would be executed by the scheduler.

You can spawn substasks during execution of `execute()`. For that:
1. Create new object of your task class
2. Call `void spawn(task *t)` to place a new task in thread queue
3. Call `void wait_for_all()` to wait for all created subtasks to finish

To execute root task call it's `execute()` method. Note that this way root task is always executed in the calling thread i.e. not in parallel.

##Example

Check `examples/fib.cpp` for a simple example.

This program calculates 35'th Fibonacci number by recurrence formula. The root task calculates 35'th number by spawning two subtasks for 34'th and 33'rd numbers. They, in turn, spawn two subtasks and so on until the first number. When subtasks finish, their sum is calculated and it is stored in parent's task memory.

More examples can be found in `tests/performance/` folder.

##Statistics

### Counters
When you compile scheduler in statistics mode it prints statistics information on termination. It includes following operation counters for each thread:

 1. `Put` -- number of tasks pushed in queue
 2. `Take` -- number of tasks popped from queue
 3. `S.Steal` -- number of successful steal operations of a single task
 4. `M.Steal` -- number of successful steal operations of multiple tasks
 5. `Take.f` -- number of failed pop operations
 6. `S.Streal.f` and `M.Streal.f` -- number of failed steal operation of a single and multiple tasks
 7. `Resize` -- number of resize operations

This counters are also printed in `scheduler_counters.tab` file.

### Queue sizes

> To use it, note that your processor must support `RDTSCP` command

If you set `-DSTACCATO_SAMPLE_DEQUES_SIZES=1` flag in statistics mode, scheduler will also create `scheduler_deque_sizes.tab` file that contains:

 - `Time` -- timestamp
 - `Deque.b_inc` -- the number of increments of bottom deque index
 - `Deque.b_dec` -- the number of decrements of bottom deque index
 - `Deque.t_inc` -- The value of top index

To collect this values the special thread is created, so it's reasonable to initialize scheduler with one less thread so that they all work in parallel.

Then you can use `misc/queues_size/main.cpp` program to calculate size change frequencies and probabilities:

```bash
g++ -Os -O3 main.cpp
./a.out scheduler_deque_sizes.tab
```
