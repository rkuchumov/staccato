Work-Stealing Task Scheduler
===================

This is my C++11 implementation of concurrent work-stealing task scheduler. It has the following features:

 1. Has only basic scheduling functions (`spawn` and `wait_for_all`)
 2. Supports weak memory model
 2. Allows to change the number of tasks removed per single steal
 3. Allows to collect statistical information about scheduler behavior

----------------

How it works
------------------

Tasks are a sets of instructions that must be executed sequentially. During the execution they can create subtasks which can be run in parallel. Thus all the tasks can be described by directed acyclic graph. 

Scheduler has a set of execution threads with private queues of tasks. When new subtasks is created (by `spawn` call) it is placed in thread's private queue. When task must wait for its subtasks to finish (i.e. `wait_for_all` is called), thread starts to execute tasks from its own queue. In case it doesn't have tasks to execute, it steals tasks from queues of others threads.

Check `/examples/fib.cpp` for a simple example.
