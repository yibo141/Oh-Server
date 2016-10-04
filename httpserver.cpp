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

int setnonblocking(int fd)
{
    int old_options = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_options | O_NONBLOCK);
    return old_options;
}

void add_sockfd(int epollfd, int sockfd, bool is_one_shot)
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

void rm_sockfd(int epollfd, int sockfd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
}

void modfd(int epollfd, int sockfd, int ev)
{
    epoll_event event;
    event.data.fd = sockfd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
}

void addsig(int signo, void (handler)(int), bool is_restart = true)
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

void show_and_send_error(int connfd, std::string msg)
{
    std::cout << msg << std::endl;
    send(connfd, msg.c_str(), msg.size(), 0);
    close(connfd);
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << "usage: " << argv[0] << "<port>" << std::endl;
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
    int user_count = 0;

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
            if(sockfd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t clientaddr_len = sizeof(clientaddr);
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr,
                        &clientaddr_len);
                if(connfd < 0)
                {
                    std::cout << "errno is: " << errno << std::endl;
                    continue;
                }
                if(user_count >= MAX_EVENTS)
                {
                    show_and_send_error(connfd, "Sorry, server is busy.");
                    continue;
                }
                add_sockfd(epollfd, connfd, true);
                user_count--;
            }
            else if(evlist[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                close(sockfd);
            else if(evlist[i].events & EPOLLIN)
            {
                std::cout << "new data from fd: " << sockfd << std::endl;
                pool->add(new http_process(epollfd, sockfd));
                user_count--;
            }
        }
    }

    /*
    while(true)
    {
        int ready = epoll_wait(epollfd, evlist, MAX_EVENTS, -1);
        if(ready == -1)
            continue;
        for(int i = 0; i < ready; ++i)
        {
            int sockfd = evlist[i].data.fd;
            if(sockfd == listenfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t clientlen = sizeof(clientaddr);
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr,
                        &clientlen);
                if(connfd < 0)
                {
                    if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        continue;
                    else
                    {
                        std::cout << "accept error" << std::endl;
                        break;
                    }
                }
                add_sockfd(epollfd, connfd, true);
            }
            else if((evlist[i].events & EPOLLHUP) ||
                    (evlist[i].events & EPOLLERR) || 
                    (!(evlist[i].events & EPOLLIN)))
            {
                std::cout << "epoll error" << std::endl;
                close(evlist[i].data.fd);
                continue;
            }
            else
            {
                std::cout << "new data from: " << evlist[i].data.fd << std::endl;
                pool->add(new http_process(evlist[i].data.fd));
            }
        }
    }
    */
    close(listenfd);
    close(epollfd);
    delete pool;
    return 0;
}
