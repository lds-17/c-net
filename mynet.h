//
// Created by Administrator on 2022/6/8.
//

#ifndef MYNET_MYNET_H
#define MYNET_MYNET_H

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<unistd.h>
#include<pthread.h>

#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define HOST "0.0.0.0"
#define PORT 9101
#define BUFFER_SIZE 256

#define MAX_EPOLL_SIZE 256

void err_exit(char *);

struct client_socket_addr {
    int sock;
    struct sockaddr addr;
};

void conn_per_thread(void);

void do_select(void);

void do_epoll(void);

void* recv_thread(void*);

int listen_sock();

int accept_sock(struct client_socket_addr *, int);

#endif //MYNET_MYNET_H
