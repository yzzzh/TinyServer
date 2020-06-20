//
// Created by yzh on 5/8/20.
//
#include <wait.h>
#include "MyProactor.h"

int MyProactor::m_sig_pipe[2] = {0};

MyProactor::MyProactor(const char* ip, short port,int worker_thread_num):m_ip(ip),m_port(port),m_worker_num(worker_thread_num){
    m_listenfd = -1;
    m_epollfd = -1;
    m_stop = false;
    m_timeout = false;
}

bool MyProactor::init() {
    if(!create_server_listener()){
        cout << "create server fail" << endl;
        return false;
    }

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, m_sig_pipe) == -1)
        return false;
    setnonblocking(m_sig_pipe[1]);
    addfd(m_epollfd, m_sig_pipe[0], EPOLLIN);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGKILL,sig_handler);
    addsig(SIGALRM,sig_handler);
    addsig(SIGPIPE, SIG_IGN);

    m_accept_thread = new std::thread(accept_thread_proc, this);
    for(int i = 0;i < m_worker_num;++i){
        m_worker_threads.push_back(new std::thread(worker_thread_proc,this));
    }

    return true;
}

bool MyProactor::create_server_listener() {
    m_listenfd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
    if(m_listenfd == -1)
        return false;

    int on = 1;
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,(char*)&on, sizeof(on));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    addr.sin_port = htons(m_port);

    if(bind(m_listenfd,(sockaddr*)&addr, sizeof(addr)) == -1)
        return false;

    if(listen(m_listenfd,50) == -1)
        return false;

    m_epollfd = epoll_create(10);
    if(m_epollfd == -1)
        return false;

//    addfd(m_epollfd,m_listenfd,EPOLLIN);

    epoll_event event;
    event.data.fd = m_listenfd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_listenfd,&event);
    setnonblocking(m_listenfd);

    return true;
}

MyProactor::~MyProactor() {}

bool MyProactor::uninit() {
    m_stop = true;

    m_accept_cond.notify_one();
    m_worker_cond.notify_all();

    m_accept_thread->join();
    delete m_accept_thread;
    for(auto t : m_worker_threads) {
        t->join();
        delete t;
    }

    epoll_ctl(m_epollfd,EPOLL_CTL_DEL,m_listenfd,0);

    close(m_listenfd);
    close(m_epollfd);
    close(m_sig_pipe[0]);
    close(m_sig_pipe[1]);
}

void MyProactor::close_conn(int sockfd){
    if(sockfd != -1) {
        m_fd2conn[sockfd]->close_conn();

        delete m_fd2conn[sockfd];
        m_fd2conn.erase(sockfd);

        m_timer.del_timer(m_fd2timer[sockfd]);

        delete m_fd2timer[sockfd];
        m_fd2timer.erase(sockfd);
    }
}

void MyProactor::sig_handler(int sig) {
    int save_errno = errno;
    send(m_sig_pipe[1], ( char* )&sig, 1, 0 );
    errno = save_errno;
}

void MyProactor::run(){
    epoll_event events[MAX_EPOLL_EVENTS];

    //alarm(TIME_SLOTS);

    printf("the server start running....\n");

    while(1){
        int epoll_num = epoll_wait(m_epollfd, events, MAX_EPOLL_EVENTS, -1);
        if(epoll_num <= 0 && errno != EINTR){
            printf("epoll failure\n");
            break;
        }

        for(int i = 0; i < epoll_num; ++i){
            int sockfd = events[i].data.fd;

            if(sockfd == m_listenfd){
                m_accept_cond.notify_one();
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                printf("connection:<%s:%hu> meets an error,closing now...\n", inet_ntoa(m_fd2conn[sockfd]->m_addr.sin_addr), ntohs(m_fd2conn[sockfd]->m_addr.sin_port));
                close_conn(sockfd);
            }
            else if(sockfd == m_sig_pipe[0] && events[i].events & EPOLLIN){
                char signals[1024];
                int sig_num = recv(m_sig_pipe[0], signals, sizeof( signals ), 0 );
                if(sig_num <= 0 )
                {
                    continue;
                }
                else
                {
                    for(int j = 0; j < sig_num; ++j )
                    {
                        switch( signals[j] )
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1,&stat,WNOHANG)) > 0){
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            case SIGKILL:
                            {
                                printf("receive SIGTERM or SIGINT or SIGKILL,stopping now...\n");
                                cout << "main loop exits..." << endl;
                                uninit();
                                return;
                                break;
                            }
                            case SIGALRM:
                            {
                                m_timeout = true;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }
            else if(events[i].events & EPOLLIN){
                update_timer(sockfd);

                if(m_fd2conn[sockfd]->read()){
                    {
                        std::lock_guard<std::mutex> lock(m_worker_mutex);
                        m_tasks.push_back(m_fd2conn[sockfd]);
                    }
                    m_worker_cond.notify_one();
                }
                else{
                    printf("connection:<%s:%hu> meets an error when reading,closing now...\n", inet_ntoa(m_fd2conn[sockfd]->m_addr.sin_addr), ntohs(m_fd2conn[sockfd]->m_addr.sin_port));
                    close_conn(sockfd);
                }
            }
            else if(events[i].events & EPOLLOUT){
                update_timer(sockfd);

                if(!m_fd2conn[sockfd]->write()){
                    printf("connection:<%s:%hu> meets an error when writing,closing now...\n", inet_ntoa(m_fd2conn[sockfd]->m_addr.sin_addr), ntohs(m_fd2conn[sockfd]->m_addr.sin_port));
                    close_conn(sockfd);
                }
            }
            else{

            }
        }

        if(m_timeout){
            timeout_handler();
            m_timeout = false;
        }
    }
    cout << "main loop exits...." << endl;
}

void MyProactor::timeout_handler() {
    m_timer.tick();
    timer_t* tmp = m_timer.top();
    if(tmp){
        alarm(std::max(0l,tmp->m_expire - std::time(0)));
    }
    else{
        alarm(TIME_SLOTS);
    }
}

void MyProactor::update_timer(int sockfd){
    timer_t* timer = m_fd2timer[sockfd];
    m_timer.del_timer(timer);
    timer->m_expire = std::time(0) + 3 * TIME_SLOTS;
    m_timer.add_timer(timer);
}

void MyProactor::timer_callback_func(http_conn* task) {
    int sockfd = task->m_sockfd;
    MyProactor* proactor = (MyProactor*)task->m_master;
    printf("connection:<%s:%hu> time out,closing now...\n", inet_ntoa(task->m_addr.sin_addr), ntohs(task->m_addr.sin_port));
    proactor->close_conn(sockfd);
}

void MyProactor::accept_thread_proc(MyProactor* proactor) {
    while(!proactor->m_stop){
        std::unique_lock<std::mutex> lock(proactor->m_accept_mutex);
        proactor->m_accept_cond.wait(lock);
        if(proactor->m_stop) break;

        sockaddr_in client_addr;
        socklen_t clinet_len;
        int connfd = accept(proactor->m_listenfd,(sockaddr*)&client_addr,&clinet_len);

        if(connfd < 0){
            printf("accept meets an error\n");
            continue;
        }
        if(proactor->m_fd2conn.size() >= MAX_CONNECTIONS){
            char err_msg[64] = "Internal server busy";
            send(connfd,err_msg,strlen(err_msg),0);
            printf("the server is too busy,reject a connection...\n");
            close(connfd);
            continue;
        }

        printf("accept a new connection:<%s:%hu>\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        addfd(proactor->m_epollfd,connfd,EPOLLIN, true);

        http_conn* task = new http_conn(proactor->m_epollfd, connfd, client_addr);
        task->m_master = (void*)proactor;
        proactor->m_fd2conn[connfd] = task;
        timer_t* timer = new timer_t(3*TIME_SLOTS,task,timer_callback_func);
        proactor->m_fd2timer[connfd] = timer;
        proactor->m_timer.add_timer(timer);

        cout << "accept thread finish a task..." << endl;
    }
    cout << "accept thread " << std::this_thread::get_id() << " exits..." << endl;
}

void MyProactor::worker_thread_proc(MyProactor * proactor) {
    while(!proactor->m_stop){
        http_conn* task;
        {
            std::unique_lock<std::mutex> lock(proactor->m_worker_mutex);
            while(proactor->m_tasks.empty()){
                if(proactor->m_stop){
                    cout << "worker thread " << std::this_thread::get_id() << " exits..." << endl;
                    return;
                }
                proactor->m_worker_cond.wait(lock);
            }
            task = proactor->m_tasks.front();
            proactor->m_tasks.pop_front();
        }
        task->process();

        cout << "worker thread finish a task..." << endl;
    }
    cout << "worker thread " << std::this_thread::get_id() << " exits..." << endl;
}