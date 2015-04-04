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

    auto left = pool.get()->async(&fib, n - 1, pool);
    auto right = pool.get()->async(&fib, n - 2, pool);

    return left.get() + right.get();
}
 
int main()
{
    pool_ptr pool(new ThreadPool(1));

    cout << fib(6, pool);

    return 0;
}
