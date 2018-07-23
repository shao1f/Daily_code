#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

void my_select(MYSQL* sql,char data[]){
    char* stu_num;
    char* phone_num;
    char* passwd;

    strtok(data,"=&");
    stu_num = strtok(NULL,"=&");
    strtok(NULL,"=&");
    passwd = strtok(NULL,"=&");

    char q[1024];
    sprintf(q,"select * from mem_info where stu_num = \"%s\" and passwd = \"%s\"",stu_num,passwd);
    mysql_query(sql,q);
    
    MYSQL_RES* res;
    res = mysql_store_result(sql);
    int row = mysql_num_rows(res);
    if(row == 0){
        printf("<html>");
        printf("<head><meta charset=\"utf-8\"><style>");
        printf("body{text-align:center;margin-left:auto;margin-right:auto;}");
        printf("</style>");
        printf("<body>");
        printf("<h1>登录失败</h1></br>");
        printf("</body>");
        printf("<body>");
        printf("<a href=\"../log_in.html\">登录失败,请返回重新登录</a>");
        printf("</body>");
        printf("</html>");
    }else {

        printf("<html>");
        printf("<head>");
        printf("<meta http-equiv=\"refresh\" content=\"1,url=../user.html\">");
        printf("<meta charset=\"utf-8\"><style>");
        printf("body{text-align:center;margin-left:auto;margin-right:auto;}");
        printf("</style>");
        printf("<body>");
        printf("<h1>登陆成功!</h1></br>");
        printf("即将为你跳转</br>");
        printf("</body>");
        printf("</html>");
    }
    mysql_free_result(res);
}

int main(){
    MYSQL sql;
    mysql_init(&sql);
    if(!mysql_real_connect(&sql,"localhost","root","qweasd","stu_union",0,NULL,0)){
        printf("Error Connect!\n");
        printf("%s\n",mysql_error(&sql));
    }

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
            data[i++] = 0;
        }
    }
    my_select(&sql,data);
}
