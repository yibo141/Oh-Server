#include "http_process.h"

http_process::http_process(const int _epollfd, const int _connfd)
{
    this->epollfd = _epollfd;
    this->connfd = _connfd;
    read_buffer = new char[READ_BUFFER_SIZE];
    write_buffer = new char[WRITE_BUFFER_SIZE];
    
    read_from_connfd();
}

http_process::~http_process()
{
    close(connfd);
    delete[] read_buffer;
    delete[] write_buffer;
}

// 重置connfd上的EPOLLONESHOT事件
void http_process::reset_oneshot()
{
    epoll_event event;
    event.data.fd = connfd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &event);
}

// 从连接套接字中读取客户请求
void http_process::read_from_connfd()
{
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
        else 
        {
            std::cout << "Receive new data from fd: " << connfd << std::endl;
            std::cout << std::endl;

            // 输出请求头
            std::cout << std::string(read_buffer) << std::endl;
            std::cout << std::endl;
            http_parser p(read_buffer);
            request = p.get_parse_result();
        }
    }
}

// 从fd中读取数据到缓冲区buffer
void http_process::read_to(const int fd, char *buffer)
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
                return;
        }
        else if(bytes_read == 0)
            break;
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
}

// 向客户发送http应答
void http_process::send_response()
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
                return;
        }
        bytes_left -= bytes_send;
        ptr += bytes_send;
    }
}

// 解析客户请求中的uri
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

// 获取客户请求的文件类型
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

// 服务客户请求的静态内容
void http_process::serve_static(const int filesize)
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
 * 服务客户请求的动态内容(暂时不支持)
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
    sprintf(body, "<html><title>Server Error</title>");
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

// 处理客户请求的入口函数
void http_process::process()
{
    if(std::string(read_buffer).size() == 0)
        return;
    // 暂时仅支持GET方法
    if(strcasecmp(request.method.c_str(), "GET"))
    {
        clienterror("501", "Not Implemented", "Server doesn't implement this method");
        return;
    }

    int is_static = parse_uri();
    struct stat sbuf;
    // 如果请求的文件不存在则发送出错信息
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
