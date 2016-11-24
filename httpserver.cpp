#include <iostream>
#include <cassert>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "http_parser.h"
#include "http_process.h"
#include "threadpool.h"

#define MAX_EVENTS 10000

// 将描述符fd设置为非阻塞
int setnonblocking(const int fd)
{
    int old_options = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_options | O_NONBLOCK);
    return old_options;
}

/*
 * 将套接字sockfd添加到epoll监听列表中
 * is_one_shot用于选择是否开启EPOLLONESHOT选项
 */
void add_sockfd(const int epollfd, const int sockfd, const bool is_one_shot)
{
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(is_one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
    setnonblocking(sockfd);
}

// 将sockfd从epoll监听列表中移除
void rm_sockfd(const int epollfd, const int sockfd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
}

// 改变在sockfd上监听的事件
void modfd(const int epollfd, const int sockfd, const int ev)
{
    epoll_event event;
    event.data.fd = sockfd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
}

// 注册信号signo的信号处理函数
void addsig(const int signo, void (handler)(int), bool is_restart = true)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if(is_restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(signo, &sa, NULL) != -1);
}

// 输出并向客户发送错误信息
void show_and_send_error(const int connfd, const std::string msg)
{
    std::cout << msg << std::endl;
    send(connfd, msg.c_str(), msg.size(), 0);
    close(connfd);
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << "usage: " << argv[0] << " <port>" << std::endl;
        return 0;
    }
    int port = atoi(argv[1]);

    addsig(SIGPIPE, SIG_IGN);

    threadpool<http_process> *pool;
    try
    {
        pool = new threadpool<http_process>();
    }
    catch(std::runtime_error e)
    {
        std::cout << e.what() << std::endl;
        delete pool;
        return -1;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int optval = 1;
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            &optval, sizeof(optval));
    assert(ret >= 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    struct epoll_event evlist[MAX_EVENTS];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    add_sockfd(epollfd, listenfd, false);

    while(true)
    {
        int ready = epoll_wait(epollfd, evlist, MAX_EVENTS, -1);
        if((ready < 0) && (errno != EINTR))
        {
            std::cout << "epoll_wait failure" << std::endl;
            break;
        }

        for(int i = 0; i < ready; ++i)
        {
            int sockfd = evlist[i].data.fd;
            // 有新的客户连接到来
            if(sockfd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t clientaddr_len = sizeof(clientaddr);
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr,
                        &clientaddr_len);
                if(connfd < 0)
                {
                    std::cout << "error: " << strerror(errno) << std::endl;
                    continue;
                }
                add_sockfd(epollfd, connfd, true);
            }
            // epoll出错
            else if(evlist[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                close(sockfd);
            // 有新的请求到来
            else if(evlist[i].events & EPOLLIN)
            {
                pool->add(new http_process(epollfd, sockfd));
            }
        }
    }

    close(listenfd);
    close(epollfd);
    delete pool;
    return 0;
}
