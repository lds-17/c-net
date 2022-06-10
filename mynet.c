//
// Created by Administrator on 2022/6/8.
//
#include "mynet.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("arg numbers wrong\n");
    } else if (strcmp(argv[1], "-h") == 0){
        printf("args: 0->thread; 1->epoll\n");
    } else if (strcmp(argv[1], "0") == 0) {
        conn_per_thread();
    } else if (strcmp(argv[1], "1") == 0) {
        do_epoll();
    } else {
        printf("args error = %s, args: 0->thread; 1->epoll\n", argv[1]);
    }

    return 0;
}

void err_exit(char *msg) {
    perror(msg);
    exit(1);
}

int listen_sock() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);   // 创建socket
    if(sockfd == -1)
        exit(1);
    struct sockaddr_in addr;    // 创建socket地址结构体, 包含此socket要监听的地址, 端口, 协议等
    int sin_len = sizeof(struct sockaddr_in);

    memset(&addr, 0, sin_len);      // 初始化结构体, 字节清零
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);    // 网络是大端序`big-endian`, 计算机是小端序`little-endian`, 需要转换;
    addr.sin_addr.s_addr = inet_addr(HOST); // 转化字符串地址为整数地址;

    int option = 1;     // 是否复用地址
    // 设置 socket 参数
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // 把socket 与 地址结构体绑定, 表示socket要监听的地址/端口, 使用的协议等;
    if (bind(sockfd, (struct sockaddr *)(&addr), sizeof(struct sockaddr_in)) < 0)
        err_exit("bind err");

    // 监听socket
    if (listen(sockfd, 128) < 0)
        err_exit("listen err");
    return sockfd;
}

int accept_sock(struct client_socket_addr *csa, int listen_sockfd) {
    // 定义客户端地址结构体, 用以新连接创建时, accept() 函数写入地址相关信息;
    struct sockaddr_in cli_addr;
    int len = sizeof(struct sockaddr_in);
    memset(&cli_addr, 0, len);
    // 定义 整型 client socket fd, 用以赋值 accept() 函数返回的新连接的 fd
    int cli_sock;
    // 调用 accept() 函数接受新连接
    if ((cli_sock = accept(listen_sockfd, (struct sockaddr *)(&cli_addr), &len)) < 0 )
        return -1;
    csa->sock = cli_sock;
    csa->addr = *(struct sockaddr *)&cli_addr;
    return 0;
}

// 每个连接一个线程处理
void conn_per_thread() {
    int sockfd = listen_sock();

    printf("[server] accepting...\n");
    // 轮询 : 每accept一个连接, 开启一个线程处理
    char *msg;
    while(1){
        // 创建 client_socket_addr 结构体并分配内存, 用以指针传入线程处理函数
        struct client_socket_addr *csa = (struct client_socket_addr *)malloc(sizeof(struct client_socket_addr));
        if (accept_sock(csa, sockfd) == -1) {
            msg = "accept error";
            break;
        }
        // 创建新线程处理新连接 cli_sock
        pthread_t tid;
        if (pthread_create(&tid, NULL, recv_thread, (void *)csa) != 0){
            msg ="create pthread error";
            break;
        }
    }
    err_exit(msg);
}

// 线程处理连接
void* recv_thread(void *arg) {
    struct client_socket_addr *csa = (struct client_socket_addr *)(arg);
    struct sockaddr_in *cli_addr = (struct sockaddr_in *)(&(csa->addr));
    char *host = inet_ntoa(cli_addr->sin_addr);
    printf("create new client pid = %ld, host = %s:%d(%d)\n", pthread_self(), host, ntohs(cli_addr->sin_port), cli_addr->sin_port);
    int sockfd = csa->sock;
    char buf[BUFFER_SIZE];
    pthread_detach(pthread_self());     // 线程转换成分离态, 线程退出时, 系统自动回收线程的资源(堆栈内存, 文件描述符等)
    while(1) {
        memset(buf, 0, BUFFER_SIZE);
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if(n < 0) {
            perror("recv error");
            free(csa);          // 释放内存
            pthread_exit(NULL);
        }
        printf("thread %d recv from [%s] msg: %s\n", sockfd, host, buf);

        // send(sockfd, "hello", 5, 0);

    }
}
