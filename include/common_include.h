//
// Created by yzh on 5/8/20.
//

#ifndef TINYSERVER_COMMON_INCLUDE_H
#define TINYSERVER_COMMON_INCLUDE_H

#include <fcntl.h>
#include <sys/epoll.h>
#include <zconf.h>
#include <cstring>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string>
#include <unordered_map>
#include <list>
#include <vector>
#include <iostream>

using std::string;
using std::list;
using std::vector;
using std::cout;
using std::endl;
using std::unordered_map;

#endif //TINYSERVER_COMMON_INCLUDE_H
