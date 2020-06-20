//
// Created by yzh on 5/3/20.
//

#ifndef TINYSERVER_CGI_CONN_H
#define TINYSERVER_CGI_CONN_H

#include "processpool.h"
#include <string>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

class cgi_conn{
public:
    const std::string root = "/var/tmp/cgi/";
    int m_epollfd;
    int m_sockfd;
    struct sockaddr_in m_addr;
    std::string m_read_buff;

    cgi_conn(int epollfd,int sockfd,const sockaddr_in& addr){
        m_epollfd = epollfd;
        m_sockfd = sockfd;
        m_addr = addr;
    }

    ~cgi_conn(){}

    void close_conn(){
        if(m_sockfd != -1){
            removefd(m_epollfd,m_sockfd);
            m_sockfd = -1;
        }
    };

    void process(){
        while(1){
            char read_buff[128] = {0};
            int bytes_read = recv(m_sockfd,read_buff, sizeof(read_buff)-1,0);
            if(bytes_read < 0){
                if(errno != EAGAIN)
                    close_conn();
                break;
            }
            else if(bytes_read == 0){
                close_conn();
                break;
            }
            else{
                m_read_buff += read_buff;
            }
        }
        printf("read buff:%s\n",m_read_buff.c_str());
        size_t pos = m_read_buff.find("\r\n");
        if(pos == std::string::npos){
            printf("the content is incomplete!\n");
            return;
        }
        std::string filename = m_read_buff.substr(0,pos);
        filename = root + filename;
        printf("processing cmd: %s...\n",filename.c_str());

        /*if(access(filename.crbegin(),F_OK) == -1){
            close_conn();
            return;
        }*/

        int pid = fork();
        if(pid == -1){
            close_conn();
            return;
        }
        else if(pid > 0){
            close_conn();
            return;
        }
        else{
            close(STDOUT_FILENO);
            dup(m_sockfd);
            execl(filename.c_str(),filename.c_str(),0);
            exit(0);
        }
    }
};


#endif //TINYSERVER_CGI_CONN_H
