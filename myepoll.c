//
// Created by Administrator on 2022/6/10.
//

#include "mynet.h"

#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 20

void setnonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0)
        err_exit("fcntl(sock, F_GETFL)");
    opts |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
        err_exit("fcntl(sock, F_SETFL, opts)");
}

// epoll
void do_epoll() {
    // 创建 listen socket fd;
    int listen_fd = listen_sock();
    // 创建 epoll fd
    int epoll_fd = epoll_create(MAX_EPOLL_SIZE);
    if (epoll_fd == -1)
        err_exit("epoll_create error");
    // 创建 epoll 监听的事件
    struct epoll_event ev;
    // 创建 epoll_event 数组, 用以调用 epoll_wait 时, 内核把有事件通知的 fd 放进此数组中;
    struct epoll_event events[MAX_EVENTS];
    int fd_counts;  // epoll_wait 返回有事件通知的 fd 的数量;
    // 设置 listen sock 事件
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    // 把 listen_fd 添加进 epoll 红黑树中监听
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
        err_exit("epoll_ctl add listen fd error");

    struct client_socket_addr csa;
    int n;
    char buf[BUFFER_SIZE];
    while (1) {
        fd_counts = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (fd_counts == -1)
            err_exit("epoll_wait error");
        for (int i = 0; i < fd_counts; ++i) {
            // 若事件 fd 是 listen_fd, 表示有新的连接请求, 调用 accept() 函数获取新连接
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                if (accept_sock(&csa, fd) == -1)
                    err_exit("epoll accept sock error");
                struct sockaddr_in *cli_addr = (struct sockaddr_in *)(&(csa.addr));
                printf("accept new client: %s:%d\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                // 设置 socket 为非阻塞
                setnonblocking(csa.sock);
                // 为新连接构造 epoll 监听事件
                ev.data.fd = csa.sock;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;  // 监听 读/写 事件, 边缘触发
                // 把新连接 fd 添加入 epoll 监听列表中
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, csa.sock, &ev) == -1)
                    err_exit("epoll_ctl add new client error");
            }
            // 由于只监听两类 socket fd: listen socket 和 connection socket, 因此以下所有分支都是 connection socket
            // 由于 connection socket 分 读/写 两类事件, 因此通过 epoll_event.events & Type 来判断事件类型
            else if (events[i].events & EPOLLIN) {    // 数据可读事件
                if ( (n = read(events[i].data.fd, buf, BUFFER_SIZE)) < 0 ) {
                    printf("socket fd:%d read error & closed\n", fd);
                    close(fd);
                    events[i].data.fd = -1;
                }
                printf("socket fd:%d EPOLLIN, recv:%s\n", fd, buf);
//                // 构造监听 写 事件, 边缘触发
//                ev.data.fd = events[i].data.fd;
//                ev.events = EPOLLOUT|EPOLLET;
//                // 修改此事件为写监听
//                epoll_ctl(epoll_fd, EPOLL_MOD, events[i].data.fd, &ev);
            } else  if (events[i].events & EPOLLOUT) {    // socket 可写事件
                // 写数据
//                write(events[i].data.fd, buf, n);
                printf("socket fd:%d EPOLLOUT\n", fd);
            } else {    // 其他事情, 不可能分支
                printf("socket fd:%d Other events: %d\n", fd, events[i].events);
            }
        }
    }
    return;
}