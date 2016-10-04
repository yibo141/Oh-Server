#include "http_process.h"

#include <iostream>
using std::cout;
using std::endl;
http_process::http_process(int _epollfd, int _connfd)
{
    this->epollfd = _epollfd;
    this->connfd = _connfd;
    read_buffer = new char[READ_BUFFER_SIZE];
    write_buffer = new char[WRITE_BUFFER_SIZE];
    read_from_connfd();
    cout << endl;
    cout << std::string(read_buffer) << endl;
    cout << endl;
    http_parser p(read_buffer);
    request = p.get_parse_result();
    /*
    cout << "-----------------------------------" << endl;
    cout << "Method: " << request.method << endl;
    cout << "URI: " << request.uri << endl;
    cout << "Version: " << request.version << endl;
    cout << "Host: " << request.host << endl;
    cout << "Connection: " << request.connection << endl;
    cout << "-----------------------------------" << endl;
    */
}

http_process::~http_process()
{
    delete[] read_buffer;
    delete[] write_buffer;
}

void http_process::reset_oneshot()
{
    epoll_event event;
    event.data.fd = connfd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &event);
}

ssize_t http_process::read_from_connfd()
{
    std::cout << "Receive new data from fd: " << connfd << std::endl;
    while(true)
    {
        int ret = recv(connfd, read_buffer, READ_BUFFER_SIZE, 0);
        if(ret == 0)
        {
            close(connfd);
            std::cout << "Foreiner closed the connection" << std::endl;
            break;
        }
        else if(ret < 0)
        {
            if(errno == EAGAIN)
            {
                reset_oneshot();
                break;
            }
        }
    }
}

ssize_t http_process::read_to(int fd, char *buffer)
{
    size_t bytes_read = 0;
    size_t bytes_left = READ_BUFFER_SIZE;
    char *ptr = buffer;
    while(bytes_left > 0)
    {
        bytes_read = read(fd, ptr, bytes_left);
        if(bytes_read == -1)
        {
            if(errno == EINTR)
                bytes_read = 0;
            else 
                return -1;
        }
        else if(bytes_read == 0)
            break;
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
    return READ_BUFFER_SIZE - bytes_left;
}

ssize_t http_process::send_response()
{
    size_t bytes_send = 0;
    size_t bytes_left = std::string(write_buffer).size();
    char *ptr = write_buffer;
    while(bytes_left > 0)
    {
        if((bytes_send = write(connfd, ptr, bytes_left)) < 0)
        {
            if(bytes_send < 0 && errno == EINTR)
                bytes_send = 0;
            else
                return -1;
        }
        bytes_left -= bytes_send;
        ptr += bytes_send;
    }
    return bytes_left;
}

int http_process::parse_uri()
{
    if(!strstr(request.uri.c_str(), "cgi-bin"))
    {
        // 请求静态内容
        filename = ".";
        filename += request.uri;
        if(request.uri[request.uri.size()-1] == '/')
            filename += "home.html";
        return 1;   // 如果请求静态内容则返回1
    }
    else
    {
        // 请求动态内容
        auto index = find(request.uri.cbegin(), request.uri.cend(), '?');
        if(index != request.uri.cend())
            cgi_args = std::string(index + 1, request.uri.cend());
        else
            cgi_args = "";

        filename = std::string(request.uri.cbegin(), index);
        filename = "." + filename;
        return 0;   // 如果请求动态内容则返回0
    }
}

void http_process::get_filetype()
{
    const char *name = filename.c_str();
    if(strstr(name, ".html"))
        filetype = "text/html";
    else if(strstr(name, ".pdf"))
        filetype = "application/pdf";
    else if(strstr(name, ".png"))
        filetype = "image/png";
    else if(strstr(name, ".gif"))
        filetype = "image/gif";
    else if(strstr(name, ".jpg"))
        filetype = "image/jpg";
    else if(strstr(name, ".jpeg"))
        filetype = "image/jpeg";
    else if(strstr(name, ".css"))
        filetype = "test/css";
    else
        filetype = "text/plain";
}

void http_process::serve_static(int filesize)
{
    get_filetype();
    sprintf(write_buffer, "HTTP/1.1 200 OK\r\n");
    sprintf(write_buffer, "%sServer: Tiny Web Server\r\n", write_buffer);
    sprintf(write_buffer, "%sContent-length: %d\r\n", write_buffer, filesize);
    sprintf(write_buffer, "%sContent_type: %s\r\n\r\n", write_buffer, filetype.c_str());
    send_response();

    int fd = open(filename.c_str(), O_RDONLY, 0);
    read_to(fd, write_buffer);
    send_response();
}

/*
void http_process::serve_dynamic()
{
    sprintf(write_buffer, "HTTP/1.1 200 OK\r\n");
    sprintf(write_buffer, "%sServer: Tiny Web Server\r\n\r\n", write_buffer);
    send_response();

    
       if(fork() == 0)
       {
       setenv("QUERY_STRING", cgi_args, 1);
       dup2(connfd, STDOUT_FILENO);
       execve(filename.c_str(), NULL, environ);
       }
       wait(NULL);
       
}
*/

/*
 * clienterror - returns an error message to the client
 */
void http_process::clienterror(const char *errnum, const char *shortmsg,
       const char *longmsg) 
{
    char body[WRITE_BUFFER_SIZE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, filename.c_str());
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(write_buffer, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    sprintf(write_buffer, "%sContent-type: text/html\r\n", write_buffer);
    sprintf(write_buffer, "%sContent-length: %d\r\n\r\n", write_buffer, (int)strlen(body));
    send_response();
    write_buffer = body;
    send_response();
}

void http_process::process()
{
    if(strcasecmp(request.method.c_str(), "GET"))
    {
        clienterror("501", "Not Implemented", "Server doesn't implement this method");
        return;
    }

    int is_static = parse_uri();
    struct stat sbuf;
    if(stat(filename.c_str(), &sbuf) < 0)
    {
        clienterror("404", "Not Found", "Server couldn't find this file");
        return;
    }

    if(is_static)
    {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror("403", "Forbidden", "Server couldn't read the file");
            return;
        }
        serve_static(sbuf.st_size);
    }
    /*
    else
    {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror("403", "Forbidden", "Server couldn't run the CGI program");
            return;
        }
        serve_dynamic();
    }
    */
}
