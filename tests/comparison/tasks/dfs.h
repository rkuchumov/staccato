#ifndef DFS_H
#define DFS_H

#include <vector>
#include <string>
#include <cmath>

#include "../test_suite.h"

using namespace std;

template<typename task, bool no_del = false>
class dfs_task: public task
{
public:
	dfs_task(int breadth_, int depth_, long *created_):
		depth(depth_), breadth(breadth_), total(created_)
	{ }

	~dfs_task() { }

	void my_execute() {
		if (depth == 0) {
			*total = 1;
			return;
		}

		vector<task *> task_list(breadth);
		vector<long> c(breadth);

		for (int j = 0; j < breadth; j++)
			task_list[j] = new(this) dfs_task<task, no_del> (breadth, depth - 1, &c[j]);

		task::exec_and_wait(task_list);

		for (int j = 0; j < breadth; j++) {
			*total += c[j];
			if (!no_del)
				delete task_list[j];
		}
	}

private:
	int depth;
	int breadth;
	long *total;
};

template<typename scheduler, typename task, bool no_del = false>
class dfs_test: public abstract_test
{
public:
	dfs_test(string name_, int breadth_ = 300, int depth_ = 3, size_t nthreads = 0) 
		: abstract_test(name_), breadth(breadth_), depth(depth_)
	{
		sh = new scheduler(nthreads);
	}

	~dfs_test() {
		delete sh;
	}

	void set_up() {
		root = new dfs_task<task, no_del> (breadth, depth, &total);
	}

	void iteration() {
		sh->runRoot(root);
	}

	bool check() {
		return total = pow(breadth, depth);
	}

	void break_down() {
		total = 0;
		
		if (!no_del)
			delete root;
	}

private:
	scheduler *sh;

	int breadth;
	int depth;
	long total;

	dfs_task<task, no_del> *root;
};

#endif /* end of include guard: DFS_H */
