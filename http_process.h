#ifndef HTTP_PROCESS_H
#define HTTP_PROCESS_H

#include <string>
#include <algorithm>

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
    http_process(int _epollfd, int _connfd);
    ~http_process();
    void process();
private:
    void reset_oneshot();
    ssize_t read_from_connfd();
    ssize_t read_to(int fd, char *buffer);
    ssize_t send_response();
    int parse_uri();
    void get_filetype();
    void serve_static(int filesize);
    void serve_dynamic();
    void clienterror(const char *errnum, const char *shortmsg, 
            const char *longmsg);

    int epollfd;
    int connfd;
    std::string filetype;
    std::string filename;
    std::string cgi_args;
    char *read_buffer;
    char *write_buffer;
    http_request request;
};

#endif // HTTP_PROCESS_H
