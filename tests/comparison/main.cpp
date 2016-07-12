#include <iostream>
#include <fstream>

#ifdef STACCATO
#include "schedulers/staccato.h"
#endif

#ifdef TBB
#include "schedulers/tbb.h"
#endif

#ifdef CILK
#include "schedulers/cilk.h"
#endif

#ifdef OPENMP
#include "schedulers/openmp.h"
#endif

#include "tasks/fib.h"
#include "tasks/dfs.h"
#include "tasks/mergesort.h"
#include "tasks/matmult.h"
#include "tasks/knapsack.h"

using namespace std;

int main()
{
	abstract_test::iterations = 20;
	abstract_test::out = new ofstream("results.tab");

#ifdef STACCATO
	{ fib_test<staccato_scheduler, staccato_task, true>("Staccato/Fib/35", 35).run(); }
	{ dfs_test<staccato_scheduler, staccato_task, true>("Staccato/DFS/300x3", 300, 3).run(); }
	{ mergesort_test<staccato_scheduler, staccato_task, true>("Staccato/MSort/23xUnif", 23).run(); }
	{ matmul_test<staccato_scheduler, staccato_task, true>("Staccato/MatMul/8", 8).run(); }
	{ knacksack_test<staccato_scheduler, staccato_task, true>("Staccato/KnackSack").run(); }
#endif

#ifdef TBB
	{ fib_test<TbbScheduler, TbbTask, true>("TBB/Fib/35", 35).run(); }
	{ dfs_test<TbbScheduler, TbbTask, true>("TBB/DFS/300x3", 300, 3).run(); }
	{ mergesort_test<TbbScheduler, TbbTask, true>("TBB/MSort/23xUnif", 23).run(); }
	{ matmul_test<TbbScheduler, TbbTask, true>("TBB/MatMul/8", 8).run(); }
	{ knacksack_test<TbbScheduler, TbbTask, true>("TBB/KnackSack").run(); }
#endif

#ifdef CILK
	{ fib_test<cilk_sheduler, cilk_task>("Cilk/Fib/35", 35).run(); }
	{ dfs_test<cilk_sheduler, cilk_task>("Cilk/DFS/300x3", 300, 3).run(); }
	{ mergesort_test<cilk_sheduler, cilk_task>("Cilk/MSort/23xUnif", 23).run(); }
	{ matmul_test<cilk_sheduler, cilk_task>("Cilk/MatMul/8", 8).run(); }
	{ knacksack_test<cilk_sheduler, cilk_task>("Cilk/KnackSack").run(); }
#endif

#ifdef OPENMP
	{ fib_test<openmp_sheduler, openmp_task>("OpenMP/Fib/35", 35).run(); }
	{ dfs_test<openmp_sheduler, openmp_task>("OpenMP/DFS/300x3", 300, 3).run(); }
	{ mergesort_test<openmp_sheduler, openmp_task>("OpenMP/MSort/23xUinf", 23).run(); }
	{ matmul_test<openmp_sheduler, openmp_task>("OpenMP/MatMul/8", 8).run(); }
	{ knacksack_test<openmp_sheduler, openmp_task>("OpenMP/KnackSack").run(); }
#endif

    return 0;
}
