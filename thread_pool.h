#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <future>

#include "worker.h"

enum state_t { ready, running, finished };
template <class T>
struct m_future
{
    state_t state;
    std::mutex state_lock;
    std::condition_variable is_finished;

    m_future(): state(ready) {};

    T data;
    task_t task;

    T get() {
        task();

        if (state != finished) {
            std::unique_lock<std::mutex> l(state_lock);
            is_finished.wait(l, [&](){return state == finished; });
        }

        return data;
    }
};


class ThreadPool
{
    workers_ptr _workers;

    worker_ptr get_rand_worker()
    {
        int i = rand() % _workers->size();
        return _workers->at(i);
    }

    worker_ptr get_worker()
    {
        auto _me = std::this_thread::get_id();
        for (auto &it : *_workers) {
            if (_me == it->get_id()) {
                return it;
            }
        }

        // std::cerr << "___\n";

        return nullptr;
    }

public:
    ThreadPool(size_t thread_cnt = 1)
    {
        _workers = workers_ptr(new std::vector<worker_ptr>());
        for (size_t i = 0; i < thread_cnt; i++) {
            worker_ptr w(new Worker(_workers));
            _workers->push_back(w);
        }

        // std::cerr << "ThreadPool is created\n";
    }

    ~ThreadPool()
    {
        // std::cerr << "ThreadPool destructor\n";

        for (auto &it : *_workers) {
            it->join();
        }
        _workers->clear();
    }

    template<class R, class F, class... Args>
    std::shared_ptr<m_future<R>> async(F f, Args... args) 
    {
        std::function<R()> func = std::bind(f, args...);
        std::shared_ptr<m_future<R>> rc(new m_future<R>());

        task_t t = [=]()
        {
            rc->state_lock.lock();
            if (rc->state == ready) {
                rc->state = running;
                rc->state_lock.unlock();

                rc->data = func();

                // rc->state_lock.lock();
                rc->state = finished;
                // rc->state_lock.unlock();
                rc->is_finished.notify_all();
            } else {
                rc->state_lock.unlock();
            }
        };
        rc->task = t;

        worker_ptr w = get_worker();
        w->assign(t);

        return rc;
    }

    template<class R, class F, class... Args>
    R exec(F f, Args... args)
    {
        std::function<R()> func = std::bind(f, args...);
        std::shared_ptr<m_future<R>> rc(new m_future<R>());

        task_t t = [=]()
        {
            rc->state_lock.lock();
            if (rc->state == ready) {
                rc->state = running;
                rc->state_lock.unlock();

                rc->data = func();

                // rc->state_lock.lock();
                rc->state = finished;
                // rc->state_lock.unlock();
                rc->is_finished.notify_all();
            } else {
                rc->state_lock.unlock();
            }
        };
        rc->task = t;

        get_rand_worker()->assign(t);

        for (auto &it : *_workers)
            it->fork();

        std::unique_lock<std::mutex> l(rc->state_lock);
        rc->is_finished.wait(l, [&](){return rc->state == finished; });

        return rc->data;
    }
};

#endif /* end of include guard: THREAD_POOL_H */
