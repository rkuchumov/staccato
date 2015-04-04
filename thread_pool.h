#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <future>

typedef std::function<void()> task_t;
class Worker;
typedef std::shared_ptr<Worker> worker_ptr;
typedef std::shared_ptr< std::vector<worker_ptr> > workers_ptr;

class Worker
{
    bool _enabled;
    std::deque<task_t> _queue;
    workers_ptr _workers;
    std::thread _thread;

    std::mutex _queue_lock;

    int _me;

    void exec()
    {
        while(1) {
            _queue_lock.lock();

            while (!_queue.empty()) {
                task_t t = _queue.front();
                _queue.pop_front();

                _queue_lock.unlock();
                // std::cerr << "Worker(" << _me << ") executing task()\n";
                t();
                _queue_lock.lock();
            }

            _queue_lock.unlock();

            if (!_enabled)
                break;

            std::this_thread::yield();
            // cerr << "Worker(" << _me << ") starts stealing\n";
            task_t t = NULL;
            int victim = rand() % _workers->size();
            if (victim == _me)
                continue;
            if (_workers->at(victim) == nullptr)
                continue;

            t = _workers->at(victim)->steal();

            if (t != NULL) {
                // cerr << _me << " has stolen task\n";
                t();
            }
        }
    }

public:
    Worker(int thread_id, workers_ptr workers):
        _enabled(false),
        _queue(),
        _workers(workers),
        _me(thread_id)
    { }

    ~Worker()
    { }

    void fork()
    {
        _enabled = true;
        _thread = std::thread(&Worker::exec, this);
    }

    void join()
    {
        // std::cerr << "Worker(" << _me << ") joning\n";
        _enabled = false;
        _thread.join();
    }

    void assign(task_t task)
    {
        // std::cerr << "Worker(" << _me << ") received task\n";

        std::unique_lock<std::mutex> l(_queue_lock);
        _queue.push_front(task);
        // std::cerr << "Worker(" << _me << ") pushed task\n";
    }

    task_t steal()
    {
        if (!_enabled)
            return NULL;

        if (!_queue_lock.try_lock())
            return NULL;

        if (_queue.empty()) {
            _queue_lock.unlock();
            return NULL;
        }

        task_t t = _queue.back();
        _queue.pop_back();

        _queue_lock.unlock();

        return t;
    }

    bool isEmpty()
    {
        std::unique_lock<std::mutex> locker(_queue_lock);
        return _queue.empty();
    }

    size_t getTaskCount()
    {
        std::unique_lock<std::mutex> locker(_queue_lock);
        return _queue.size();
    }

};

class ThreadPool
{
    workers_ptr _workers;

    worker_ptr getRandWorker()
    {
        int i = rand() % _workers->size();
        return _workers->at(i);
    }

public:
    ThreadPool(size_t thread_cnt = 1)
    {
        _workers = workers_ptr(new std::vector<worker_ptr>());
        for (size_t i = 0; i < thread_cnt; i++) {
            worker_ptr w(new Worker(i, _workers));
            _workers->push_back(w);
        }

        for (auto &it : *_workers) {
            it->fork();
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

    // void exec(task_t t) {
    //     getRandWorker()->assign(t);
    // }

    template<class F, class... Args>
    auto async(F f, Args... _args)
    -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared< std::packaged_task<return_type()> > (bind(f, _args...));

        std::future<return_type> rc = task->get_future();
        
        getRandWorker()->assign([task](){(*task)(); });

        return rc;
    }
};

#endif /* end of include guard: THREAD_POOL_H */

