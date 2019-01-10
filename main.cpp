#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/file.h>
#include <strings.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>
#include "lucasthread/lucasthread.h"
#include "lucashttp/lucashttp.h"

int daemonize();
int startup(u_short *);
void error_die(const char *);

#define THREAD 64

int main(void)
{
    /* 切换为后台守护进程 */
    if(daemonize() != 0)
    {
        return -1;
    }
    int server_sock = -1;
    /* 在此处设置端口 */
    u_short port = 12450;
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);

    /* 在对应端口建立 http 服务 */
    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);
    /* 创建线程池 */
    LucasThreadPool threadPool(THREAD);
    printf("Pool started with %d threads\n", THREAD);

    while (1)
    {
        client_sock = accept(server_sock,(struct sockaddr *)&client_name,&client_name_len);
        if (client_sock == -1)
            error_die("accept");
        /* 每次的新连接加入到线程池的任务队列中，供空闲线程调用函数 */
        if (threadPool.add(&accept_request, (void*)&client_sock) != 0)
            printf( "Job add error." );
    }

    threadPool.stop();
    close(server_sock);

    return(0);
}

/**********************************************************************/
/* 在特定端口上监听网络连接
 * 如果port为0则随机生成
 * 否则在port上监听 */
/**********************************************************************/
int startup(u_short *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    /* 建立 socket */
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    /* 如果当前指定端口是 0，则随机分配一个端口 */
    if (*port == 0)
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    /* 开始监听 */
    if (listen(httpd, 5) < 0)
        error_die("listen");
    /* 返回 socket id */
    return(httpd);
}

/**********************************************************************/
/* 错误信息 */
/**********************************************************************/
void error_die(const char *sc)
{
    /* 出错信息处理 */
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* 将http服务转至后台，切断和terminal的关联，
 * 使之在terminal关闭后仍然可以运行 */
/**********************************************************************/
int daemonize()
{
    int i, fd;
    pid_t pid;


    /* 首先占据输入 输出 错误三个流 */
    open("/dev/zero", O_RDWR);
    open("/dev/zero", O_RDWR);
    open("/dev/zero", O_RDWR);

    /* 单实例锁 */
    if((fd = open("/var/lock/test.lck", O_RDWR | O_CREAT | O_CLOEXEC | O_NOCTTY, 0666)) == -1 || \
            flock(fd, LOCK_EX | LOCK_NB) == -1)
    {
        return -1;
    }

    /* 后面要fork 此处首先避免僵尸进程 */
    signal(SIGCHLD, SIG_IGN);

    /* 父进程退出, 新进程一定不是所在进程组的头领进程 */
    /* 非头领进程才可执行setsid, 创建新会话 */
    if((pid = fork()) < 0)
    {
        //perror( "fork" );
        return -1;
    }
    else if (pid > 0)
    {
        exit(0);
    }

    /* 子进程创建新会话, 成为整个会话的头领进程, 无控制终端 */
    setsid();

    /* 会话头领进程会再次获得终端, 进行二次fork, 生成新进程一定非头领进程 */
    /* 在fork前先屏蔽SIGHUP, 否则当会话头领进程退出, 会话所有子进程会收到该信号 */
    signal(SIGHUP, SIG_IGN);
    if((pid = fork()) < 0)
    {
        //perror( "fork" );
        //fflush( stderr );
        return -1;
    }
    else if(pid > 0)
    {
        exit(0);
    }

    /* 屏蔽所有信号 */
    for(i = 1; i <= SIGRTMAX; i ++)
    {
        signal(i, SIG_IGN);
    }

    /* 设置为根目录, 无法卸载 */
    // chdir( "/" );

    /* 清文件创建屏蔽字 */
    umask(0);

    /* 关闭所有已打开文件描述符 */
    for(i = 0; i < 1024; i ++)
    {
        if(i != fd)
        {
            close(i);
        }
    }

    /* 重定向输入、输出、错误三个流为日志 必然为0 1 2 后面普通的输出函数皆定向到该文件 */
    fd = open("/log.txt", O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
    if(fd != 0)
    {
        return -1;
    }

    dup(0);
    dup(0);

    return 0;
}
