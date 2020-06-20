//
// Created by yzh on 4/30/20.
//

#ifndef TINYSERVER_TIMEWHEEL_TIMER_H
#define TINYSERVER_TIMEWHEEL_TIMER_H

#include <ctime>
#include <vector>
#include <list>

using std::vector;
using std::list;

template<typename T>
class timewheel_timer;

template <typename T>
class tw_timer{
public:
    tw_timer *next,*prev;
    std::time_t m_expire;
    T* m_task;
    void (*m_callback_func)(T*);
    int m_time_slot;
    int m_rotation;

    tw_timer(std::size_t delay,T* task=NULL,void (*cb_func)(T*)=NULL){
        next = prev = NULL;
        m_expire = delay + std::time(NULL);
        m_task = task;
        m_callback_func = cb_func;
        m_time_slot = -1;
        m_rotation = -1;
    }

    void set_task(T* task){
        m_task = task;
    }

    void set_callback_func(void (*cb_func)(T*)){
        m_callback_func = cb_func;
    }
};

template <typename T>
class timewheel_timer{
public:
    typedef tw_timer<T> timer_t;

    timewheel_timer():m_cur_slot(0){
        for(int i = 0;i < SLOT_SIZE;++i){
            m_slots[i] = NULL;
        }
    }

    ~timewheel_timer(){
        for(int i = 0;i < SLOT_SIZE;++i){
            timer_t* temp = m_slots[i];
            while(temp){
                m_slots[i] = temp->next;
                delete temp;
                temp = m_slots[i];
            }
        }

    };

    void add_timer(timer_t* timer){
        int ticks = (timer->m_expire - std::time(NULL)) / SI;
        if(ticks < 0) ticks = 0;
        int rots = ticks / SLOT_SIZE;
        int ts = (m_cur_slot + (ticks % SLOT_SIZE)) % SLOT_SIZE;

        timer->m_rotation = rots;
        timer->m_slot = ts;

        if(m_slots[ts]){
            timer->next = m_slots[ts];
            m_slots[ts]->prev = timer;
            m_slots[ts] = timer;
        }
        else{
            m_slots[ts] = timer;
        }
    }

    void del_timer(timer_t* timer){
        if(!timer) return;

        int ts = timer->m_time_slot;
        if(timer == m_slots[ts]){
            m_slots[ts] = timer->next;
            if(m_slots[ts])
                m_slots[ts]->prev = NULL;
            delete timer;
            timer = NULL;
        }
        else{
            timer->prev->next = timer->next;
            if(timer->next)
                timer->next->prev = timer->prev;
            delete timer;
            timer = NULL;
        }
    }

    void tick(){
        timer_t* temp = m_slots[m_cur_slot];
        while(temp){
            if(temp->m_rotation > 0){
                --temp->m_rotation;
                temp = temp->next;
            }
            else{
                temp->m_callback_func(temp->m_task);
                timer_t* temp2 = temp->next;
                del_timer(temp);
                temp = temp2;
            }
        }
        m_cur_slot = (m_cur_slot + 1) % SLOT_SIZE;
    }

private:
    static const int SLOT_SIZE = 60;
    static const int SI = 1;

    timer_t* m_slots[SLOT_SIZE];
    int m_cur_slot;

};

#endif //TINYSERVER_TIMEWHEEL_TIMER_H
