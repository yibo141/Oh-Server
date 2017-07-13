## Oh-Server

一个用C++写的简单的epoll多线程http服务器。

`httpserver.cpp`                      主函数文件

`locker.cpp`和`locker.h`              互斥锁、信号量和条件变量的封装类

`threadpool.h`                        线程池类模板

`http_parser.cpp`和`http_parser.h`    解析http请求的类

`http_process.cpp`和`http_process.h`  处理http请求的类



## Usage

`make`

`./Oh-Server <port>`

## More details

这是我以前写的一个版本，我又把它重写了，新建了一个repo在隔壁的`Servant`。更多详细信息请参考[我的博客](http://www.cnblogs.com/broglie/p/5931375.html)。

