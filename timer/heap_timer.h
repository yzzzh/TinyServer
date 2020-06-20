//
// Created by yzh on 5/1/20.
//

#ifndef TINYSERVER_HEAP_TIMER_H
#define TINYSERVER_HEAP_TIMER_H

#include <ctime>
#include <queue>
#include <vector>

template <typename T>
class h_timer{
public:
    std::time_t m_expire;
    T* m_task;
    void (*m_callback_func)(T*);

    h_timer(int delay,T* task = NULL,void (*cb_func)(T*)=NULL){
        m_expire = std::time(NULL) + delay;
        m_task = task;
        m_callback_func = cb_func;
    }

    void set_task(T* task){
        m_task = task;
    }

    void set_callback_func(void (*cb_func)(T*)){
        m_callback_func = cb_func;
    }
};



template <typename T>
class heap_timer{
public:
    typedef h_timer<T> timer_t;

    class cmp{
        bool operator()(timer_t* timer1,timer_t* timer2){
            return timer1->m_expire > timer2->m_expire;
        }
    };

    std::priority_queue<timer_t*,std::vector<timer_t*>,cmp> m_heap;

public:
    heap_timer(){}
    ~heap_timer(){}

    void add_timer(timer_t* timer){
        if(!timer) return;
        m_heap.push(timer);
    }

    void del_timer(timer_t* timer){
        if(!timer) return;
        timer->m_callback_func = NULL;
    }

    void pop_timer(){
        if(m_heap.empty())
            return;
        m_heap.pop();
    }

    timer_t* top(){
        if(m_heap.empty())
            return NULL;
        return m_heap.top();
    }

    void tick(){
        std::time_t time_cur = std::time(NULL);
        while(!m_heap.empty()){
            timer_t* temp = m_heap.top();

            if(temp->m_expire <= time_cur){
                if(temp->m_callback_func){
                    temp->m_callback_func(temp->m_task);
                }
                m_heap.pop();
            }
            else{
                break;
            }
        }
    }

};

#endif //TINYSERVER_HEAP_TIMER_H
