#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

void insert(MYSQL sql,char data[]){
    char* stu_num;
    char* phone_num;
    char* passwd;
    char* again_passwd;

    strtok(data,"=&");
    stu_num = strtok(NULL,"=&");
    strtok(NULL,"=&");
    phone_num = strtok(NULL,"=&");
    strtok(NULL,"=&");
    passwd = strtok(NULL,"=&");
    strtok(NULL,"=&");
    again_passwd = strtok(NULL,"=&");

    char q[1024];
    sprintf(q,"insert into mem_info values(\"%s\",\"%s\",\"%s\")",stu_num,phone_num,passwd);
    /* printf("%s",q); */
    mysql_query(&sql,q);


    printf("<html>");
    printf("<head><meta charset=\"utf-8\"><style>");
    printf("body{text-align:center;margin-left:auto;margin-right:auto;}");
    printf("</style>");
    printf("<body>");
    printf("<h1>注册成功!</h1></br>");
    printf("<table border=\"1\" align=\"center\">");
    printf("<tr>");
    printf("<td>你的账号</td>");
    printf("<td>%s</td>",stu_num);
    printf("</tr>");
    printf("<tr>");
    printf("<td>你的手机号</td>");
    printf("<td>%s</td>",phone_num);
    printf("</tr>");
    printf("<tr>");
    printf("<td>你的密码</td>");
    printf("<td>%s</td>",passwd);
    printf("</tr>");
    printf("</table>");
    /* printf("<h1>stu_num:%s phone_num:%s passwd:%s </h1></br>",stu_num,phone_num,passwd); */
    printf("</body>");
    printf("<body>");
    printf("<a href=\"../log_in.html\">注册成功,点击前往登录</a>");
    printf("</body>");
    printf("</html>");
}

int main(){
    MYSQL sql;
    mysql_init(&sql);
    if(!mysql_real_connect(&sql,"localhost","root","qweasd","stu_union",0,NULL,0)){
        printf("Error connecting!\n");
        printf("%s\n",mysql_error(&sql));
        return 1;
    }
    /* printf("connect success!\n"); */
    char* method;
    char* query_str;
    int content_length;

    char data[1024];
    int i = 0;
    if((method = getenv("METHOD_ENV"))){
        if(strcasecmp(method,"GET") == 0){
            query_str = getenv("QUERY_STR_ENV");
            strcpy(data,query_str);
        }else {
            content_length = atoi(getenv("CONTENT_LENGTH"));
            for(;i<content_length;++i){
                read(0,&data[i],1);
            }
            data[i] = 0;
        }
    }
    insert(sql,data);
}
