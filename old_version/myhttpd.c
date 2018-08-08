#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "threadpool.h"

#define MAX 1024

static void usage(const char* msg){
    printf("Usage: %s [port]\n",msg);
}

static int startup(int port){
    int sock = socket(AF_INET,SOCK_STREAM,0);//创建套接字
    if(sock < 0){ //创建失败,直接返回
        perror("socket");
        return 2;
    }
    
    //取消timewait,地址复用
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in server;
    server.sin_family = AF_INET;                //ipv4协议
    server.sin_port = htons(port);              // 端口号
    server.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置为所有本机地址

    if(bind(sock,(struct sockaddr*)&server,sizeof(server)) < 0){//绑定套接字
        perror("bind");
        return 3;
    }

    if(listen(sock,5) < 0){//加入监听
        perror("listen");
        return 4;
    }

    //走到这里说明套接字已经加入监听
    return sock;
}

static int get_line(int sock,char line[],int size){
    //这里是为了统一格式,将\r\n替换成我们需要的格式
    int c = 'a';
    int n = 0;
    int i = 0;
    while(i<size-1 && c != '\n'){
        n = recv(sock,&c,1,0);
        if(n > 0){
            if(c == '\r'){
                recv(sock,&c,1,MSG_PEEK);
                if(c == '\n'){
                    recv(sock,&c,1,0);
                }else {
                    c = '\n';
                }
            }
            line[i++] = c;
        }else {
            c = '\n';
        }
    }
    line[i] = '\0';
    return i;
}

static int clear_header(int sock){
    //清除sock中剩余信息,循环读取即可
    char buf[MAX];
    int ret = -1;
    do{
        ret = get_line(sock,buf,sizeof(buf));
        /* printf("%s",buf); */
    }while(ret > 0 && strcmp(buf,"\n") != 0);

    return ret;
}

static void echo_404(int sock){
    char err_path[MAX] = "wwwroot/err_404.html";
    int fd = open(err_path,O_RDONLY);
    struct stat st;
    char buf[MAX];
    stat(err_path,&st);

    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");//响应报头
    send(sock,buf,strlen(buf),0);

    sendfile(sock,fd,0,st.st_size);
    close(fd);

}

static void echo_err(int sock,int errCode){
    clear_header(sock);
    switch(errCode){
    case 404:
        echo_404(sock);
        break;
    default:
        break;
    }
}

void echo_www(int sock,char path[],int size){
    clear_header(sock);     //清除请求报头
    char buf[MAX];
    int fd = open(path,O_RDONLY);

    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");//响应报头
    send(sock,buf,strlen(buf),0);

    sendfile(sock,fd,0,size);
    close(fd);

}

int exe_cgi(int sock,char method[],char path[],char* query_string){
    int content_lenth = -1;         //post方法需要知道请求正文长度
    char buf[MAX];
    //将以下参数设为环境变量是因为在执行程序替换后,环境变量是可以被大家都能看到的
    char method_env[MAX/32];        //请求方法环境变量
    char query_string_env[MAX];     //get方法参数环境变量
    char content_length_env[MAX];   //请求正文长度环境变量

    if(strcasecmp(method,"GET") == 0){
        clear_header(sock);
    }else{
        do{
            memset(buf,'\0',sizeof(buf));
            get_line(sock,buf,sizeof(buf));
            if(strncmp(buf,"Content-Length:",strlen("Content-Length:")) == 0){
                /* content_lenth = atoi(buf+16); */
                content_lenth = atoi(&buf[16]);
            }
        }while(strcmp(buf,"\n") != 0);
        if(content_lenth == -1){
            perror("POST");
            return 404;
        }
    }

    memset(buf,'\0',sizeof(buf));
    //响应报头
    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
    send(sock,buf,strlen(buf),0);

    int input[2]={0,0};//输入管道
    int output[2]={0,0};//输出管道

    if(pipe(input) < 0){
        perror("pipe input");
        return 404;
    }
    if(pipe(output) < 0){
        perror("pipe output");
        return 404;
    }
    pid_t id = fork(); //创建子进程来进行程序替换
    if(id < 0){ // fork失败了
        perror("fork");
        return 404;
    }else if(id == 0){//子进程
        close(input[1]);
        close(output[0]);

        dup2(input[0],0);//old new  新的描述符和旧的保持一致
        dup2(output[1],1);
        //导入环境变量
        memset(method_env,'\0',sizeof(method_env));
        memset(query_string_env,'\0',sizeof(query_string_env));
        memset(content_length_env,'\0',sizeof(content_length_env));

        sprintf(method_env,"METHOD_ENV=%s",method);
        putenv(method_env);
        if(strcasecmp(method,"GET") == 0){
            sprintf(query_string_env,"QUERY_STR_ENV=%s",query_string);
        printf("method_env = %s,query_string_env = %s\n",method_env,query_string_env);
            putenv(query_string_env);
        }else {
            sprintf(content_length_env,"CONTENT_LENGTH=%d",content_lenth);
            putenv(content_length_env);
        }

        //进程替换
        execl(path,path,NULL);
        exit(-1); //替换失败直接退出
    }else {//父进程
        close(input[0]);
        close(output[1]);

        int c;
        if(strcasecmp(method,"POST") == 0){
            int i = 0;
            for(;i<content_lenth;++i){
                recv(sock,&c,1,0);//从 sock中读取一个字符
                write(input[1],&c,1);//把读出来的数据写到cgi的标准输入里
            }
        }
        while(read(output[0],&c,1) > 0){
            send(sock,&c,1,0);
        }
        close(input[1]);
        close(output[0]);
        waitpid(id,NULL,0) ;
    }
    return 200;
}

void* http_responce(void* arg){
    int* psock = (int*)arg;
    int sock = *psock;
    char buf[MAX];          //读取首行,进行分析
    int errCode = 200;      //状态码
    int cgi = 0;            //是否以cgi方式运行
    char method[MAX/32];    //保存请求方法
    char path[MAX];         //保存请求的资源路径
    char url[MAX];          //保存请求的url
    char *query_string = NULL;     //保存GET方法的参数
    size_t i = 0;//method的下标
    size_t j = 0;//buf的下标

    memset(method,'\0',sizeof(method));
    memset(path,'\0',sizeof(path));
    memset(url,'\0',sizeof(url));
    memset(buf,'\0',sizeof(buf));

    int ret = get_line(sock,buf,sizeof(buf)); //获取首行,进行分析
    if(ret < 0){
        errCode = 404;
        goto end;
    }

    //获取请求方法
    while((i<sizeof(method)-1) && (j<sizeof(buf)) && !(isspace(buf[j]))){
        method[i] = buf[j];
        ++i;
        ++j;
    }
    method[i] = '\0';

    if(strcasecmp(method,"GET") != 0 && strcasecmp(method,"POST") != 0){
        errCode = 404;
        goto end;
    }

    if(strcasecmp(method,"POST") == 0){//POST方法,需要使用cgi模式运行
        cgi = 1;
    }

    while(isspace(buf[j]) && j<sizeof(buf)){//防止有多个空格的情况
        ++j;
    }

    i = 0;//url下标,接下来处理请求url
    while((i<sizeof(url)-1) && (j<sizeof(buf)) && !(isspace(buf[j]))){
        url[i] = buf[j];
        ++i;
        ++j;
    }
    url[i] = '\0';

    if(strcasecmp(method,"GET") == 0){//GET方法
        query_string = url;
        while(*query_string != '\0' && *query_string != '?'){
            query_string++;
        }
        if(*query_string == '?'){//有参数的GET方法,参数放在query_string里,用cgi模式执行
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path,"wwwroot%s",url);//此时的url里就是路径

    if(path[strlen(path)-1] == '/'){//如果请求的是根目录,就将根目录后加上主页
        strcat(path,"index.html");
        }

    struct stat st;//获取文件属性
    if(stat(path,&st) < 0){//获取失败,返回错误信息
        errCode = 404;
        goto end;
    }else {//获取成功
        if(S_ISDIR(st.st_mode)){//文件是不是目录,是目录则在后面拼上首页
            strcpy(path,"wwwroot/index.html");
        }else if((st.st_mode & S_IXOTH) || (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP)){
            //是可执行程序,调用cgi
            cgi = 1;
        }else {
            //TODO
            //是普通文件
        }
        if(cgi){
            //执行cgi
            exe_cgi(sock,method,path,query_string);
        }else {//非cgi,直接返回首页
            echo_www(sock,path,st.st_size);
        }
    }

end:
    if(errCode != 200){
        echo_err(sock,errCode);
    }
    close(sock);
    return NULL;
}

int main(int argc,char* argv[]){
    daemon(1,0);
    pool_init(5);//最大线程数为5个
    if(argc != 2){
        usage(argv[0]);
        return 1;
    }
    signal(SIGPIPE,SIG_IGN);

    int listen_sock = startup(atoi(argv[1])); // 监听套接字

    for(;;){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int new_sock = accept(listen_sock,(struct sockaddr*)&client,&len); //接受链接,创建新的套接字

        pool_add_work(http_responce,(void*)&new_sock);//线程池中添加任务
    }

    return 0;
}