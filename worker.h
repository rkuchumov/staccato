#ifndef WORKER_H
#define WORKER_H

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <future>
#include <unistd.h>

#include "deque.h"

typedef std::function<void()> task_t;
class Worker;
typedef std::shared_ptr<Worker> worker_ptr;
typedef std::shared_ptr< std::vector<worker_ptr> > workers_ptr;

class Worker
{
    bool _enabled;
    // std_deque<task_t> _queue;
    // b_deque<task_t> _queue;
    u_deque<task_t> _queue;
    workers_ptr _workers;
    std::thread _thread;

    std::mutex _queue_lock;
    std::thread::id _me;

    void exec()
    {
        _me = std::this_thread::get_id();
        while (1) {
            task_t t = _queue.pop_bottom();
            while (t != NULL) {
                t();
                t = _queue.pop_bottom();
            }

            if (!_enabled)
                break;

            int victim = rand() % _workers->size();
            if (_workers->at(victim) == nullptr)
                continue;

            t = _workers->at(victim)->steal();

            if (t != NULL) {
                t();
            }
        }
    }

public:
    Worker(workers_ptr workers):
        _enabled(false),
        _queue(),
        _workers(workers)
    { }

    ~Worker()
    { }

    std::thread::id get_id()
    {
        return _me;
    }

    void fork()
    {
        _enabled = true;
        _thread = std::thread(&Worker::exec, this);
    }

    void join()
    {
        _enabled = false;
        _thread.join();
    }

    void assign(task_t task)
    {
        _queue.push_bottom(task);
    }

    task_t steal()
    {
        if (!_enabled)
            return NULL;

        if (_me == std::this_thread::get_id())
            return NULL;

        return _queue.pop_top();

    }
};


#endif /* end of include guard: WORKER_H */

