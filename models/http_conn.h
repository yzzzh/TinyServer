//
// Created by yzh on 4/28/20.
//

#ifndef TINYSERVER_HTTP_CONN_H
#define TINYSERVER_HTTP_CONN_H

#include "common_include.h"

class http_conn{
public:
    static const string m_root;
    static const int FILENAME_LEN = 256;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD{
        GET = 0,POST,HEAD,PUT,DELETE,OPTION
    };
    enum READ_STATE{
        READ_REQUESTLINE = 0,READ_HEADER,READ_CONTENT,READ_DONE
    };
    enum LINE_STATE{
        LINE_CONTINUE = 0,LINE_BAD,LINE_OK
    };
    enum HTTP_CODE{
        HTTP_CONTINUE = 0,HTTP_OK,HTTP_BAD_REQUEST,HTTP_FORBIDDEN,HTTP_INTERNAL_ERROR,HTTP_NOT_FOUND,HTTP_PAST
    };

    static std::unordered_map<HTTP_CODE ,std::pair<string,string>> m_http_code_description;
    static std::unordered_map<HTTP_CODE,string> m_http_code_map;

    int m_epollfd;
    int m_sockfd;
    sockaddr_in m_addr;
    void* m_master;

private:
    std::string m_read_buff;
    std::string m_write_buff;

    READ_STATE m_read_state;
    HTTP_CODE m_code;

    std::string m_method;
    std::string m_url;
    std::string m_version;
    std::string m_target_file;
    size_t m_filesize;
    std::string m_host;

    int m_content_length;
    bool m_keepalive;

public:
    http_conn(int epollfd,int fd,const sockaddr_in& addr);
    ~http_conn();

    bool read();
    bool write();
    void close_conn();
    void process();

private:
    void init();
    void init_map();
    void reset();
    http_conn::HTTP_CODE check_file();

    http_conn::HTTP_CODE process_read();
    void process_write(HTTP_CODE);

    LINE_STATE get_line(std::string&);
    http_conn::HTTP_CODE parse_requestline(const std::string&);
    http_conn::HTTP_CODE parse_headers(const std::string&);
    http_conn::HTTP_CODE parse_content(const std::string&);

};

#endif //TINYSERVER_HTTP_CONN_H
