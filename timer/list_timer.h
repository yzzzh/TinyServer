//
// Created by yzh on 5/2/20.
//

#ifndef TINYSERVER_LIST_TIMER_H
#define TINYSERVER_LIST_TIMER_H

#include <ctime>

template <typename T>
class lst_timer{
public:
    std::time_t m_expire;
    T* m_task;
    lst_timer *prev,*next;
    void (*m_callback_func)(T*);

    lst_timer(int delay,T* task = NULL,void (*cb_func)(T*)=NULL):prev(NULL),next(NULL){
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
class list_timer{
    typedef lst_timer<T> timer_t;
private:
    timer_t *m_head,*m_tail;
public:
    list_timer(): m_head(NULL), m_tail(NULL){}
    ~list_timer(){
        timer_t* temp = m_head;
        while(temp){
            m_head = temp->next;
            delete temp;
            temp = m_head;
        }
    }

    void add_timer(timer_t* timer){
        if(!timer)
            return;
        if(!m_head)
            m_head = m_tail = timer;

        if(timer->m_expire <= m_head->m_expire){
            timer->next = m_head;
            m_head->prev = timer;
            m_head = timer;
            return;
        }

        add_timer(timer, m_head);
    }

    void del_timer(timer_t* timer){
        if(!timer)
            return ;
        if(timer == m_head && timer == m_tail){
            delete timer;
            m_head = m_tail = NULL;
            return;
        }

        if(timer == m_head){
            m_head = m_head->next;
            m_head->prev = NULL;
            delete timer;
            timer = NULL;
            return;
        }

        if(timer == m_tail){
            m_tail = m_tail->prev;
            m_tail->next = NULL;
            delete timer;
            timer = NULL;
            return ;
        }

        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
        timer = NULL;
        return ;
    }

    void adjust_timer(timer_t* timer){
        if(!timer) return;

        timer_t* temp = timer->next;
        if(!temp || temp->m_expire >= timer->m_expire)
            return;

        if(timer == m_head){
            m_head = m_head->next;
            m_head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, m_head);
        }
        else{
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
            add_timer(timer,timer->next);
        }
    }

    void add_timer(timer_t* timer,timer_t* head){
        timer_t* prev = head;
        timer_t* temp = prev->next;

        while(temp && temp->m_expire < timer->m_expire){
            prev = prev->next;
            temp = temp->next;
        }

        if(!temp){
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            m_tail = timer;
        }
        else{
            prev->next = timer;
            timer->next = temp;
            temp->prev = timer;
            timer->prev = prev;
        }
    }

    void tick(){
        if(!m_head) return;

        std::time_t time_cur = std::time(NULL);
        timer_t* temp = m_head;
        while(temp){
            if(temp->m_expire > time_cur)
                break;

            temp->m_callback_func(temp->m_task);
            m_head = temp->next;
            if(m_head)
                m_head->prev = NULL;
            delete temp;
            temp = m_head;
        }
    }
};

#endif //TINYSERVER_LIST_TIMER_H
