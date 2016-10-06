#ifndef HTTP_PROCESS_H
#define HTTP_PROCESS_H

#include <string>
#include <algorithm>
#include <iostream>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "http_parser.h"

#define READ_BUFFER_SIZE  2048
#define WRITE_BUFFER_SIZE  1024

// 网站根目录
// std::string root_directory = "/var/www/html";

class http_process
{
public:
    http_process(const int _epollfd, const int _connfd);
    ~http_process();
    
    // 处理客户请求的入口函数
    void process();
private:
    // 重置connfd上的EPOLLONESHOT事件
    void reset_oneshot();

    // 从connfd中读取客户请求
    void read_from_connfd();
    
    // 从fd中读取数据到缓冲器buffer中
    void read_to(const int fd, char *buffer);

    // 向客户发送响应
    void send_response();

    // 解析客户请求的uri
    int parse_uri();

    // 获取客户请求的文件类型
    void get_filetype();

    // 服务静态内容
    void serve_static(const int filesize);

    // 服务动态内容(暂不支持)
    void serve_dynamic();

    // 向客户发送错误信息
    void clienterror(const char *errnum, const char *shortmsg, 
            const char *longmsg);

    int epollfd;
    int connfd;
    std::string filetype;  // 客户请求的文件类型
    std::string filename;  // 客户请求的文件名
    std::string cgi_args;  // cgi的参数
    char *read_buffer;     // 读缓冲区
    char *write_buffer;    // 写缓冲区，用于存储将要发送到客户的数据
    http_request request;  // 存储解析后的客户http请求
};

#endif // HTTP_PROCESS_H
