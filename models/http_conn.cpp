//
// Created by yzh on 4/28/20.
//

#include "http_conn.h"
#include "utils.h"
#include "common_include.h"
#include <sstream>

const string http_conn::m_root = "/var/tmp/html";
std::unordered_map<http_conn::HTTP_CODE ,std::pair<string,string>> http_conn::m_http_code_description = std::unordered_map<http_conn::HTTP_CODE ,std::pair<string,string>>();
std::unordered_map<http_conn::HTTP_CODE ,string> http_conn::m_http_code_map = std::unordered_map<http_conn::HTTP_CODE ,string>();

void http_conn::init_map(){
    m_http_code_description[HTTP_OK] = {ok_200_title, ok_200_content};
    m_http_code_description[HTTP_BAD_REQUEST] = {err_400_title, err_400_content};
    m_http_code_description[HTTP_FORBIDDEN] = {err_403_title, err_403_content};
    m_http_code_description[HTTP_NOT_FOUND] = {err_404_title, err_404_content};
    m_http_code_description[HTTP_INTERNAL_ERROR] = {err_500_title, err_500_content};

    m_http_code_map[HTTP_OK] = "200";
    m_http_code_map[HTTP_BAD_REQUEST] = "400";
    m_http_code_map[HTTP_FORBIDDEN] = "403";
    m_http_code_map[HTTP_NOT_FOUND] = "404";
    m_http_code_map[HTTP_INTERNAL_ERROR] = "500";
}

http_conn::http_conn(int epollfd,int fd,const sockaddr_in& addr)
: m_epollfd(epollfd), m_sockfd(fd), m_addr(addr)
{
    init();
}

http_conn::~http_conn() {}

void http_conn::init() {
    init_map();

    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    addfd(m_epollfd, m_sockfd, true);

    reset();
}

void http_conn::close_conn() {
    if(m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        close(m_sockfd);
        m_sockfd = -1;
    }
}

void http_conn::reset() {
        m_read_state = READ_REQUESTLINE;
        m_code = HTTP_CONTINUE;

        m_keepalive = false;
        m_content_length = 0;

        m_method = "GET";
        m_url = "";
        m_version = "";
        m_host = "";
        m_target_file = "";
        m_filesize = 0;

        m_read_buff = "";
        m_write_buff = "";

        m_master = NULL;
}

void http_conn::process() {
    if(m_read_state != READ_DONE) {
        printf("processing read buff...\n");
        HTTP_CODE ret = process_read();
        if(ret == HTTP_CONTINUE){
            printf("the request is incomplete...\n");
            modfd(m_epollfd, m_sockfd,EPOLLIN);
            return;
        }
        else{
            m_read_state = READ_DONE;
            if(ret == HTTP_OK){
                m_code = HTTP_OK;
            }
            else{
                m_keepalive = false;
            }
            printf("processing write buff...\n");
            process_write(ret);
            modfd(m_epollfd, m_sockfd, EPOLLOUT);
        }
    }
}

bool http_conn::read() {
    while(1){
        char read_buff[128] = {0};
        int bytes_read = recv(m_sockfd, read_buff, sizeof(read_buff) - 1, 0);
        if(bytes_read == -1){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                return true;
            }
            else{
                return false;
            }
        }
        else if(bytes_read == 0){
            return false;
        }

        printf("read from connection:\n%s\n",read_buff);
        m_read_buff += read_buff;

        if(m_read_buff.size() > READ_BUFFER_SIZE) return false;
    }
}

bool http_conn::write() {

    while(1){
        int bytes_write = send(m_sockfd, m_write_buff.c_str(), m_write_buff.size(), 0);
        if(bytes_write == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return true;
            }
            else{
                return false;
            }
        }
        else if(bytes_write == 0){
            return false;
        }

        printf("write to connection:\n%s",m_write_buff.substr(0,bytes_write).c_str());
        m_write_buff.erase(0,bytes_write);

        if(m_write_buff.empty()) break;
    }

    if(m_code == HTTP_OK) {
        int tar_fd = open(m_target_file.c_str(), O_RDONLY);
        sendfile(m_sockfd, tar_fd, NULL, m_filesize);
        close(tar_fd);
    }

    if(m_keepalive){
        reset();
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return true;
    }
    else{
        return false;
    }
}

http_conn::HTTP_CODE http_conn::process_read() {
    std::string s;
    LINE_STATE line_state;
    while((line_state = get_line(s)) == LINE_OK){
        //printf("parse line:%s\n",s.c_str());
        HTTP_CODE ret;
        switch (m_read_state){
            case READ_REQUESTLINE:
                ret = parse_requestline(s);
                if(ret == HTTP_PAST){
                    m_read_state = READ_HEADER;
                    continue;
                }
                else{
                    return ret;
                }
                break;
            case READ_HEADER:
                ret = parse_headers(s);
                if(ret == HTTP_PAST){
                    if(m_content_length == 0){
                        m_read_state = READ_DONE;
                        return HTTP_OK;
                    }
                    else {
                        m_read_state = READ_CONTENT;
                        continue;
                    }
                }
                else if(ret == HTTP_CONTINUE){
                    continue;
                }
                else{
                    return ret;
                }
                break;
            case READ_CONTENT:
                ret = parse_content(s);
                if(ret == HTTP_PAST){
                    m_read_state = READ_DONE;
                    return HTTP_OK;
                }
                else{
                    continue;
                }
                break;
        }
    }
    if(line_state == LINE_BAD)
        return HTTP_BAD_REQUEST;
    else
        return HTTP_CONTINUE;
}

void http_conn::process_write(http_conn::HTTP_CODE code) {
    std::stringstream ss;
    ss << "HTTP/1.1 " << m_http_code_map[code] << " " << m_http_code_description[code].first << "\r\n";
    if(code == HTTP_OK){
        ss << "Content-Length: " << m_filesize << "\r\n";
    }
    else{
        ss << "Content-Length: " << m_http_code_description[code].second.size() << "\r\n";
    }
    ss << "Connection: " << (m_keepalive?"keep-alive":"close") << "\r\n";
    ss << "\r\n";

    if(code != HTTP_OK){
        ss << m_http_code_description[code].second;
    }

    m_write_buff = ss.str();
}

http_conn::LINE_STATE check(const std::string& s){
    size_t pos_r = s.find('\r');
    size_t pos_n = s.find('\n');
    if(pos_r == string::npos && pos_n == string::npos)
        return http_conn::LINE_CONTINUE;
    else if(pos_r != string::npos && pos_n == string::npos){
        if(pos_r == s.size() - 1)
            return http_conn::LINE_CONTINUE;
        else{
            return http_conn::LINE_BAD;
        }
    }
    else if(pos_r == string::npos && pos_n != string::npos){
        return http_conn::LINE_BAD;
    }
    else{
        if(pos_r + 1 == pos_n){
            return http_conn::LINE_OK;
        }
        else{
            return http_conn::LINE_BAD;
        }
    }

}

http_conn::LINE_STATE http_conn::get_line(std::string & s) {

    if(m_read_state != READ_CONTENT) {
        LINE_STATE ret = check(m_read_buff);
        //printf("ret:%d\n",ret);
        if(ret == LINE_BAD || ret == LINE_CONTINUE){
            return ret;
        }

        size_t pos = m_read_buff.find("\r\n");
        s = m_read_buff.substr(0,pos);
        m_read_buff.erase(0,pos+2);

        return LINE_OK;
    }
    else{
        if(m_read_buff.size() < m_content_length){
            return LINE_CONTINUE;
        }
        else{
            s = std::move(m_read_buff);
            m_read_buff = "";
            return LINE_OK;
        }
    }
}

http_conn::HTTP_CODE http_conn::check_file(){
    m_target_file = m_root + m_url;
    struct stat file_stat;

    if(stat(m_target_file.c_str(),&file_stat) < 0){
        return HTTP_NOT_FOUND;
    }
    if(!(file_stat.st_mode & S_IROTH)){
        return HTTP_FORBIDDEN;
    }
    if(S_ISDIR(file_stat.st_mode)){
        return HTTP_BAD_REQUEST;
    }
    m_filesize = file_stat.st_size;
    return HTTP_PAST;
}

http_conn::HTTP_CODE http_conn::parse_requestline(const std::string & s) {
    std::string method;
    std::string url;
    std::string version;
    std::stringstream ss(s);
    ss >> method >> url >> version;

    if(method == "GET"){
        m_method = "GET";
    }
    else{
        return HTTP_BAD_REQUEST;
    }

    if(version == "HTTP/1.1"){
        m_version = version;
    }
    else{
        return HTTP_BAD_REQUEST;
    }

    if(!url.empty()){
        m_url = url;
    }
    else{
        return HTTP_BAD_REQUEST;
    }

    return check_file();
}


http_conn::HTTP_CODE http_conn::parse_headers(const std::string & s) {
    if(s.empty()) return HTTP_PAST;

    int pos = s.find(':');
    std::string key = trim(s.substr(0,pos));
    std::string value = trim(s.substr(pos+1));

    if(key == "Connection"){
        if(value == "keep-alive"){
            m_keepalive = true;
        }
        else{
            m_keepalive = false;
        }
    }
    else if(key == "Content-Length"){
        m_content_length = atoi(value.c_str());
    }
    else if(key == "Host"){
        m_host = value;
    }
    else{
        std::cout << "Unknown header:" << key << std::endl;
        return HTTP_BAD_REQUEST;
    }

    return HTTP_CONTINUE;
}

http_conn::HTTP_CODE http_conn::parse_content(const std::string & s) {
    if (s.size() >= m_content_length){
        return HTTP_PAST;
    }
    else{
        return HTTP_CONTINUE;
    }
}

