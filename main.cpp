#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>
#include <unistd.h>
using namespace std;

#include "thread_pool.h"

typedef shared_ptr<ThreadPool> pool_ptr;

int fib(int n, pool_ptr pool) {
    if (n <= 2)
        return 1;

    auto left = pool.get()->async<int>(&fib, n - 1, pool);
    auto right = pool.get()->async<int>(&fib, n - 2, pool);

    return left->get() + right->get();
}

int fib2(int n)
{
    if (n <= 2) return 1;
    int x = 1;
    int y = 1;
    int ans = 0;
    for (int i = 2; i < n; i++) {
        ans = x + y;
        x = y;
        y = ans;
    }
    return ans;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    pool_ptr pool(new ThreadPool(3));

    int n = atoi(argv[1]);
    int r = pool.get()->exec<int>(&fib, n, pool);
    cerr << "fib(" << n << ") = " << r << " ";
    if (r == fib2(n))
        cerr << "OK\n";
    else
        cerr << "FUCK\n";

    return 0;
}
