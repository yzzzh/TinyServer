//
// Created by yzh on 5/8/20.
//

#ifndef TINYSERVER_UTILS_H
#define TINYSERVER_UTILS_H

#include "common_include.h"

static const string ok_200_title = "OK";
static const string ok_200_content = "";
static const string err_400_title = "Bad Request";
static const string err_400_content = "Bad Request";
static const string err_403_title = "Forbidden";
static const string err_403_content = "You have no permission";
static const string err_404_title = "Not Found";
static const string err_404_content = "404 Not Found";
static const string err_500_title = "Internal Error";
static const string err_500_content = "Internal Error";

static int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

static void addfd(int epollfd,int fd,int e,bool one_shot=false){
    epoll_event event;
    event.data.fd = fd;
    event.events = e | EPOLLET | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

static void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
}

static void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

static string trim(const string& s){
    int left = 0,right = s.size()-1;
    while(s[left] == ' ') ++left;
    while(s[right] == ' ') --right;
    if(left > right){
        return "";
    }
    else{
        return s.substr(left,right-left+1);
    }
}


static void addsig(int sig,void(handler)(int),bool restart = true){
    struct sigaction sa;
    memset(&sa,0, sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    sigaction(sig,&sa,NULL);
}

#endif //TINYSERVER_UTILS_H
