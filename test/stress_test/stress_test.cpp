//
// Created by yzh on 5/6/20.
//

#include <cstring>
#include <iostream>
#include "../../include/common_include.h"

static const char* request = "GET /index.html HTTP/1.1\r\nConnection:keep-alive\r\nContent-Length:0\r\n\r\n";

int setnonblock(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLHUP;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblock(fd);
}

void modfd(int epollfd,int sockfd,int e){
    epoll_event event;
    event.data.fd = sockfd;
    event.events = e | EPOLLET | EPOLLRDHUP  | EPOLLERR | EPOLLHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,sockfd,&event);
}

void removefd(int epollfd,int sockfd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,sockfd,0);
}

bool m_write(int sockfd,std::string& s){
    int bytes_write = 0;
    while(1){
        bytes_write = send(sockfd,s.c_str(),s.size(),0);
        if(bytes_write == 0){
            return false;
        }
        else if(bytes_write == -1){
            if(errno == EAGAIN){
                sleep(1);
                continue;
            }
            else{
                return  false;
            }
        }
        else{
            s.erase(0,bytes_write);
        }
        if(s.empty())
            return true;
    }
}

bool m_read(int sockfd,std::string& s){
    int bytes_read = 0;
    while(1){
        char read_buff[128] = {0};
        bytes_read = recv(sockfd,read_buff, sizeof(read_buff)-1,0);
        if(bytes_read == 0)
            return false;
        else if(bytes_read == -1){
            if(errno == EAGAIN){
                return true;
            }
            else{
                return false;
            }
        }
        else{
            s += read_buff;
        }
    }
}

void start_conn(int epollfd,int num,const char* ip,int port){
    struct sockaddr_in addr;
    memset(&addr,0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    for(int i = 0;i < num;++i){
        sleep(1);
        int sockfd = socket(AF_INET,SOCK_STREAM,0);
        printf("create socket:%d\n",i+1);
        if(sockfd < 0)
            continue;
        if(connect(sockfd,(sockaddr*)&addr, sizeof(addr)) == 0){
            printf("succeed to connect:%d\n",i+1);
            addfd(epollfd,sockfd);
        }
    }
}

void close_conn(int epollfd,int sockfd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,sockfd,0);
    close(sockfd);
}

using std::cout;
using std::endl;

int main(int argc,char** argv){
    if(argc != 4){
        printf("usage : %s ip port num\n",argv[0]);
        return 1;
    }
    int epollfd = epoll_create(100);
    start_conn(epollfd,atoi(argv[3]),argv[1],atoi(argv[2]));
    epoll_event events[10000];
    int cnt = atoi(argv[3]);
    while(1){
        int num = epoll_wait(epollfd,events,10000,-1);
        for(int i = 0;i < num;++i){
            int sockfd = events[i].data.fd;
            if(events[i].events & EPOLLIN){
                std::string read_buff;
                if(!m_read(sockfd,read_buff)){
                    --cnt;
                    printf("sock %d fail to read,closing now...\n",sockfd);
                    close_conn(epollfd,sockfd);
                }
                printf("socket %d succeed to read from server:\n%s\n",sockfd,read_buff.c_str());
                modfd(epollfd,sockfd,EPOLLOUT);
                sleep(3);
            }
            else if(events[i].events & EPOLLOUT){
                std::string req = request;
                if(!m_write(sockfd,req)){
                    --cnt;
                    printf("sock %d fail to write,closing now...\n",sockfd);
                    close_conn(epollfd,sockfd);
                }
                printf("socket %d succeed to send request to server!\n",sockfd);
                modfd(epollfd,sockfd,EPOLLIN);
                sleep(3);
            }
            else{
                --cnt;
                printf("sock %d lose connection,closing now...\n",sockfd);
                close_conn(epollfd,sockfd);
            }
        }
        if(cnt == 0) break;
    }
    cout << "all connections closed,done" << endl;
}