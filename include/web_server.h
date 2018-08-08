#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wait.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/* #define NORMAL 1 */
/* #define WARNING 2 */
/* #define FATAL 3 */

#define MAX_SIZE 1024

/* static int log_fd = open("./err_log.log",O_WRONLY|O_APPEND); */ 

void print_log(const char* msg,const char* level);//打印日志

static int get_line(int sock,char* buf,int size);//获取新连接socket的一行内容

static int clear_header(int sock);//清除新连接中剩余所有信息

static void echo_404(int sock);//返回404页面

static void echo_err(int sock,int errCode);//根据错误码返回不同的页面

static void echo_www(int sock,char path[],int size);//返回站点首页

static int exe_cgi(int sock,char method[],char path[],char* query_string);//cgi程序

void* server_handle(void* arg);//每一个线程将要的执行的任务

class Config{//配置文件类
public:
    Config();
    bool get_config();
    const char* Ip();
    int Port();
    int Daemon();
    int Max_thread();
    void Close();
    ~Config();
private:
    int _fd;//配置文件
    std::string _ip;//IP地址
    int _port;//端口号
    int _daemon;//是否守护
    int _max_thread;//线程池最大线程数
};

class Listen_Sock{//创建监听套接字
public:
    Listen_Sock();
    ~Listen_Sock();
    bool Init(Config& config);
    int listen_sock();
private:
    int _listen_sock;
    int _opt;
    struct sockaddr_in _server;
};
