//
// Created by yzh on 5/1/20.
//

#ifndef TINYSERVER_TREE_TIMER_H
#define TINYSERVER_TREE_TIMER_H

#include <ctime>
#include <map>
#include <set>

template <typename T>
struct t_timer{
    std::time_t m_expire;
    T* m_task;
    void (*m_callback_func)(T*);

    t_timer(int delay,T* task = NULL,void (*cb_func)(T*) = NULL){
        m_expire = std::time(0) + delay;
        m_task = task;
        m_callback_func = cb_func;
    }
};

template <typename T>
class tree_timer{
public:
    typedef t_timer<T> timer_t;

    tree_timer(){}
    ~tree_timer(){};


    void add_timer(timer_t* timer){
        if(!timer) return;
        m_tree.insert(timer);
    }

    void del_timer(timer_t* timer){
        if(!timer) return;
        m_tree.erase(timer);
    }

    void pop_timer(){
        if(m_tree.empty())
            return;
        m_tree.erase(m_tree.begin());
    }

    timer_t* top(){
        if(m_tree.empty()) return NULL;
        return *m_tree.begin();
    }

    void tick(){
        while(!m_tree.empty()){
            std::time_t time_cur = std::time(NULL);

            timer_t* temp = top();

            if(temp->m_expire > time_cur)
                break;

            temp->m_callback_func(temp->m_task);

            pop_timer();
        }

    }

private:
    struct cmp{
        bool operator()(timer_t* t1,timer_t* t2){
            return t1->m_expire < t2->m_expire;
        }
    };

    std::multiset<timer_t*,cmp> m_tree;
};

#endif //TINYSERVER_TREE_TIMER_H
