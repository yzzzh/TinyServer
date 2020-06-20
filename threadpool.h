//
// Created by yzh on 4/28/20.
//

#ifndef TINYSERVER_THREADPOOL_H
#define TINYSERVER_THREADPOOL_H

#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class threadpool{

public:
    threadpool(int thread_num = 8, int max_requests = 10000): m_thread_num(thread_num), m_max_requests(max_requests), m_stop(false){
        for(int i = 0;i < m_thread_num;++i){
            threads.push_back(new std::thread(worker,this));
        }
    }

    ~threadpool(){
        m_stop = true;
        m_cond.notify_all();
        for(auto* t:threads) {
            t->join();
            delete t;
        }
    }

    bool add_task(T* task){
        std::lock_guard<std::mutex> guard(m_mutex);
        if(task_queue.size() >= m_max_requests)
            return false;
        task_queue.push_back(task);
        m_cond.notify_one();
        return true;
    }

    void run(){
        while(!m_stop){
            T* task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                while (task_queue.empty()) {
                    if (m_stop) {
                        return;
                    }
                    m_cond.wait(lock);
                }
                task = task_queue.front();
                task_queue.pop_front();
            }
            if(task) {
                printf("thread %lu get a task...\n",std::this_thread::get_id());
                task->process();
            }
        }
    }

private:
    static void worker(void* args){
        threadpool* pool = (threadpool*) args;
        pool->run();
    }

    int m_thread_num;
    int m_max_requests;
    std::list<T*> task_queue;
    std::list<std::thread*> threads;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_stop;

};

#endif //TINYSERVER_THREADPOOL_H
