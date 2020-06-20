//
// Created by yzh on 5/8/20.
//

#ifndef TINYSERVER_MYPROACTOR_H
#define TINYSERVER_MYPROACTOR_H

#include "common_include.h"
#include "utils.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include "http_conn.h"
#include "tree_timer.h"

#define MAX_EPOLL_EVENTS 1024
#define MAX_CONNECTIONS 10000
#define TIME_SLOTS 10

class MyProactor{
private:
    string m_ip;
    short m_port;

    int m_epollfd;
    int m_listenfd;
    bool m_stop;

    int m_worker_num;

    std::thread* m_accept_thread;
    list<std::thread*> m_worker_threads;

    std::mutex m_accept_mutex;
    std::condition_variable m_accept_cond;

    std::mutex m_worker_mutex;
    std::condition_variable m_worker_cond;

    list<http_conn*> m_tasks;

    unordered_map<int,http_conn*> m_fd2conn;

    typedef tree_timer<http_conn>::timer_t timer_t;

    tree_timer<http_conn> m_timer;
    bool m_timeout;
    unordered_map<int,timer_t*> m_fd2timer;

    static int m_sig_pipe[2];

public:
    MyProactor(const char* ip,short port,int worker_thread_num = 8);
    ~MyProactor();

    void run();
    bool init();
    bool uninit();

private:
    MyProactor(const MyProactor&);
    MyProactor& operator=(const MyProactor&);

    bool create_server_listener();
    static void worker_thread_proc(MyProactor*);
    static void accept_thread_proc(MyProactor*);
    void timeout_handler();
    void update_timer(int sockfd);
    static void timer_callback_func(http_conn*);

    static void sig_handler(int sig);

    void close_conn(int sockfd);
};

#endif //TINYSERVER_MYPROACTOR_H
