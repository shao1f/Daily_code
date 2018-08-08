#include "../include/web_server.h"

void print_log(const char* err_msg,const char* level){//日志处理函数
    int log_fd = open("log/err_log.log",O_WRONLY|O_APPEND);
    time_t time_p;
    time(&time_p);
    char err_buf[1024];
    memset(err_buf,0x00,sizeof(err_buf));
    char cur_time[strlen(ctime(&time_p))];
    strncpy(cur_time,ctime(&time_p),strlen(ctime(&time_p))-1);
    cur_time[strlen(ctime(&time_p))-1] = '\0';
    sprintf(err_buf,"[%s]:%s\t\t<%s>\n",cur_time,err_msg,level);
    write(log_fd,err_buf,strlen(err_buf));
    close(log_fd);
}

Config::Config()//服务器配置的初始化
    :_fd(-1),
     _ip("0"),
     _port(8080),
     _daemon(1),
     _max_thread(5)
{}

Config::~Config(){}

bool Config::get_config(){//获得配置文件信息
    _fd = open("../config/web_server.conf",O_RDONLY|O_CREAT);
    if(_fd == -1){
        print_log("error to open config file!","FATAL");
        return false;
    }
    char buf[MAX_SIZE];
    ssize_t s = read(_fd,buf,sizeof(buf));
    if(s > 0){
        buf[s] = '\0';
        char* default_config = strstr(buf,"DEFAULT_IP:");
        strtok(default_config,":\r\n");
        _ip = strtok(NULL,":\r\n");
        strtok(NULL,":\r\n");
        _port = atoi(strtok(NULL,":\r\n"));
        strtok(NULL,":\r\n");
        _daemon = atoi(strtok(NULL,":\r\n"));
        strtok(NULL,":\r\n");
        _max_thread = atoi(strtok(NULL,":\r\n"));
        
        /* std::cout<<"ip"<<ip<<"port"<<port<<"demon"<<demon<<"thread"<<max_thread<<std::endl; */
    }
    else{
        print_log("read config error!","WARNING");
        return false;
    }
    return true;
}

const char* Config::Ip(){//获得IP
    return _ip.c_str();
}

int Config::Port(){//获得端口号
    return _port;
}

int Config::Daemon(){//是否为守护进程运行
    return _daemon;
}

int Config::Max_thread(){//最大线程数
    return _max_thread;
}

void Config::Close(){
    if(_fd!=-1){
        close(_fd);
    }
}

Listen_Sock::Listen_Sock()//构造
    :_listen_sock(-1),
     _opt(1)
{
}

Listen_Sock::~Listen_Sock(){//资源清理
    if(_listen_sock!=-1){
        close(_listen_sock);
    }
}

bool Listen_Sock::Init(Config& config){//listen_socket的初始化
    _listen_sock = socket(AF_INET,SOCK_STREAM,0);
    if(_listen_sock == -1){
        print_log("socket error!","FATAL");
        return false;
    }
    //端口复用
    setsockopt(_listen_sock,SOL_SOCKET,SO_REUSEADDR,&_opt,sizeof(_opt));

    _server.sin_family = AF_INET;
    _server.sin_port = htons(config.Port());
    if(strcmp(config.Ip(),"0") == 0){//应对不同的配置信息
        _server.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        _server.sin_addr.s_addr = inet_addr(config.Ip());
    }
    
    if(bind(_listen_sock,(struct sockaddr*)&_server,sizeof(_server)) < 0){
        print_log("bind error!","FATAL");
        return false;
    }
    if(listen(_listen_sock,5) < 0){
        print_log("listen error!","FATAL");
        return false;
    }
    return true;
}

int Listen_Sock::listen_sock(){//获取listen_sock
    return _listen_sock;
}

static int get_line(int sock,char* buf,int size){//按行读取socket
    int c = 'a';
    int ret = 0;
    int i = 0;
    while(i<size && c !='\n'){
        ret = recv(sock,&c,1,0);
        if(ret > 0){
            if(c == '\r'){
                recv(sock,&c,1,MSG_PEEK);//MSG_PEEK,不会真正拿走,只是看一下
                if(c == '\n'){
                    recv(sock,&c,1,0);
                }else {
                    c = '\n';
                }
            }
            buf[i++] = c;
        }else {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
}

static int clear_header(int sock){
    //清除sock中剩余信息,循环读取即可
    char buf[MAX_SIZE];
    int ret = -1;
    do{
        ret = get_line(sock,buf,sizeof(buf));
        /* printf("%s",buf); */
    }while(ret > 0 && strcmp(buf,"\n") != 0);

    return ret;
}

static void echo_404(int sock){//错误界面
    char err_path[MAX_SIZE] = "wwwroot/err_404.html";
    int fd = open(err_path,O_RDONLY);
    struct stat st;
    char buf[MAX_SIZE];
    stat(err_path,&st);

    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");//响应报头
    send(sock,buf,strlen(buf),0);

    sendfile(sock,fd,0,st.st_size);
    close(fd);
}

static void echo_err(int sock,int errCode){//状态处理
    clear_header(sock);
    switch(errCode){
    case 404:
        echo_404(sock);
        break;
    default:
        break;
    }
}

static void echo_www(int sock,char path[],int size){//首页返回
    clear_header(sock);     //清除请求报头
    char buf[MAX_SIZE];
    int fd = open(path,O_RDONLY);
    if(fd < 0){
        print_log("error file!","WARNING");
        return;
    }

    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");//响应报头
    send(sock,buf,strlen(buf),0);

    if(sendfile(sock,fd,0,size)<0){
        print_log("sendfile error!","WARNING");
        close(fd);
        return;
    }
    close(fd);
    return;
}

static int exe_cgi(int sock,char method[],char path[],char* query_string){//动态cgi程序
    int content_lenth = -1;         //post方法需要知道请求正文长度
    char buf[MAX_SIZE];
    //将以下参数设为环境变量是因为在执行程序替换后,环境变量是可以被大家都能看到的
    char method_env[MAX_SIZE/32];        //请求方法环境变量
    char query_string_env[MAX_SIZE];     //get方法参数环境变量
    char content_length_env[MAX_SIZE];   //请求正文长度环境变量

    if(strcasecmp(method,"GET") == 0){
        clear_header(sock);
    }else{
        do{
            memset(buf,'\0',sizeof(buf));
            get_line(sock,buf,sizeof(buf));
            //获取到body的内容长度信息,即content-length
            if(strncmp(buf,"Content-Length:",strlen("Content-Length:")) == 0){
                /* content_lenth = atoi(buf+16); */
                content_lenth = atoi(&buf[16]);
            }
        }while(strcmp(buf,"\n") != 0);
        if(content_lenth == -1){
            print_log("content-length error!","WARNING");
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
        print_log("pip input error!","FATAL");
        return 404;
    }
    if(pipe(output) < 0){
        print_log("pip output error!","FATAL");
        return 404;
    }
    pid_t id = fork(); //创建子进程来进行程序替换
    if(id < 0){ // fork失败了
        print_log("fork error!","FATAL");
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
        close(sock);
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
    close(sock);
    return 200;
}

void* server_handle(void* arg){
    /* int* psock = (int*)arg; */
    /* int sock = *psock; */
    int sock = *((int*)arg);
    char buf[MAX_SIZE];          //读取首行,进行分析
    int errCode = 200;      //状态码
    int cgi = 0;            //是否以cgi方式运行
    char method[MAX_SIZE/32];    //保存请求方法
    char path[MAX_SIZE];         //保存请求的资源路径
    char url[MAX_SIZE];          //保存请求的url
    char *query_string = NULL;     //保存GET方法的参数
    size_t i = 0;//method的下标
    size_t j = 0;//buf的下标

    memset(method,'\0',sizeof(method));
    memset(path,'\0',sizeof(path));
    memset(url,'\0',sizeof(url));
    memset(buf,'\0',sizeof(buf));

    int ret = get_line(sock,buf,sizeof(buf)); //获取首行,进行分析
    if(ret < 0){
        print_log("get line error!","FATAL");
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
        print_log("method error!","FATAL");
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
        print_log("path is not exist!","FATAL");
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
            errCode = exe_cgi(sock,method,path,query_string);
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
