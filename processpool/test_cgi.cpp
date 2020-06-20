//
// Created by yzh on 5/3/20.
//
#include "processpool.h"
#include "cgi_conn.h"

int main(int argc,char** argv){
    if(argc <= 2){
        printf("usage: %s <ip> <port>\n",argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in addr;
    memset(&addr,0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    bind(listenfd,(struct sockaddr*)&addr, sizeof(addr));

    listen(listenfd,10);

    processpool<cgi_conn>* pool = processpool<cgi_conn>::create(listenfd);
    pool->run();

    delete pool;
    close(listenfd);
}